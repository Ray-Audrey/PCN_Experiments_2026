// EXP 8: TCP and UDP over a shared bottleneck (NS-3.42)
#include <fstream>
#include <iostream>
#include <string>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
using namespace ns3;

int main(int argc, char *argv[]) {
    std::string bw = "2Mbps", delay = "10ms", udpRate = "1Mbps";
    uint32_t q = 50;
    CommandLine cmd(__FILE__);
    cmd.AddValue("bw", "Bottleneck bandwidth", bw);
    cmd.AddValue("delay", "Bottleneck delay", delay);
    cmd.AddValue("udpRate", "UDP source rate", udpRate);
    cmd.AddValue("q", "Bottleneck FIFO queue size in packets", q);
    cmd.Parse(argc, argv);

    // 1. Nodes and links: H0,H1 -- R0 -- bottleneck -- R1 -- H2,H3
    NodeContainer hosts; hosts.Create(4);
    NodeContainer routers; routers.Create(2);
    PointToPointHelper access, bottleneck;
    access.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    access.SetChannelAttribute("Delay", StringValue("2ms"));
    access.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize", StringValue("50p"));
    bottleneck.SetDeviceAttribute("DataRate", StringValue(bw));
    bottleneck.SetChannelAttribute("Delay", StringValue(delay));
    bottleneck.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize", StringValue(std::to_string(q) + "p"));
    NetDeviceContainer h0r0 = access.Install(hosts.Get(0), routers.Get(0));
    NetDeviceContainer h1r0 = access.Install(hosts.Get(1), routers.Get(0));
    NetDeviceContainer r0r1 = bottleneck.Install(routers.Get(0), routers.Get(1));
    NetDeviceContainer r1h2 = access.Install(routers.Get(1), hosts.Get(2));
    NetDeviceContainer r1h3 = access.Install(routers.Get(1), hosts.Get(3));

    // Explicit FIFO root queue on the bottleneck, so its configured capacity is meaningful.
    // 2. Internet stack and unique subnets
    InternetStackHelper internet; internet.Install(hosts); internet.Install(routers);
    Ipv4AddressHelper ip;
    ip.SetBase("10.1.1.0", "255.255.255.0"); ip.Assign(h0r0);
    ip.SetBase("10.1.2.0", "255.255.255.0"); ip.Assign(h1r0);
    ip.SetBase("10.1.3.0", "255.255.255.0"); ip.Assign(r0r1);
    ip.SetBase("10.1.4.0", "255.255.255.0"); Ipv4InterfaceContainer h2If = ip.Assign(r1h2);
    ip.SetBase("10.1.5.0", "255.255.255.0"); Ipv4InterfaceContainer h3If = ip.Assign(r1h3);
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    // 3. TCP flow: host 0 -> host 3, port 5000
    PacketSinkHelper tcpSink("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 5000));
    ApplicationContainer tcpSinkApp = tcpSink.Install(hosts.Get(3));
    tcpSinkApp.Start(Seconds(0)); tcpSinkApp.Stop(Seconds(20));
    BulkSendHelper tcpSource("ns3::TcpSocketFactory", InetSocketAddress(h3If.GetAddress(1), 5000));
    tcpSource.SetAttribute("SendSize", UintegerValue(1024));
    tcpSource.SetAttribute("MaxBytes", UintegerValue(0));
    ApplicationContainer tcpApp = tcpSource.Install(hosts.Get(0));
    tcpApp.Start(Seconds(1)); tcpApp.Stop(Seconds(20));

    // 4. UDP flow: host 1 -> host 2, port 5001
    PacketSinkHelper udpSink("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), 5001));
    ApplicationContainer udpSinkApp = udpSink.Install(hosts.Get(2));
    udpSinkApp.Start(Seconds(0)); udpSinkApp.Stop(Seconds(20));
    OnOffHelper udpSource("ns3::UdpSocketFactory", InetSocketAddress(h2If.GetAddress(1), 5001));
    udpSource.SetConstantRate(DataRate(udpRate), 512);
    ApplicationContainer udpApp = udpSource.Install(hosts.Get(1));
    udpApp.Start(Seconds(1)); udpApp.Stop(Seconds(20));

    // 5. Traces and monitor
    access.EnablePcapAll("exp8-access"); bottleneck.EnablePcapAll("exp8-bottleneck");
    FlowMonitorHelper fmh; Ptr<FlowMonitor> monitor = fmh.InstallAll();
    Simulator::Stop(Seconds(20)); Simulator::Run(); monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(fmh.GetClassifier());

    // 6. Metrics and CSV
    std::ofstream csv("./scratch/results_exp8.csv", std::ios::app);
    if (csv.tellp() == 0) csv << "Flow,Port,Bandwidth,LinkDelay,QueuePackets,UDPConfiguredRate,ThroughputMbps,DelayMs,PDRPercent,LossPercent\n";
    for (const auto &entry : monitor->GetFlowStats()) {
        const auto &s = entry.second;
        auto tuple = classifier->FindFlow(entry.first);
        if (tuple.destinationPort != 5000 && tuple.destinationPort != 5001) continue;
        if (s.txPackets == 0) continue;
        double loss = 100.0 * (s.txPackets - s.rxPackets) / s.txPackets;
        double pdr = 100.0 * s.rxPackets / s.txPackets;
        double throughput = 0, avgDelay = 0;
        if (s.rxPackets > 0) {
            double duration = (s.timeLastRxPacket - s.timeFirstTxPacket).GetSeconds();
            if (duration > 0) throughput = s.rxBytes * 8.0 / duration / 1e6;
            avgDelay = s.delaySum.GetSeconds() * 1000.0 / s.rxPackets;
        }
        csv << (tuple.destinationPort == 5000 ? "TCP" : "UDP") << ',' << tuple.destinationPort << ','
            << bw << ',' << delay << ',' << q << ',' << udpRate << ',' << throughput << ','
            << avgDelay << ',' << pdr << ',' << loss << '\n';
    }
    csv.close(); monitor->SerializeToXmlFile("flow-results-exp8.xml", true, true);
    Simulator::Destroy(); return 0;
}

/* RUN (delete the previous CSV before a new parameter sweep)
rm -f results_exp8.csv
./ns3 run "scratch/exp8_tcp_udp_bottleneck --bw=2Mbps --delay=10ms --udpRate=1Mbps --q=50"
./ns3 run "scratch/exp8_tcp_udp_bottleneck --bw=5Mbps --delay=10ms --udpRate=1Mbps --q=50"
./ns3 run "scratch/exp8_tcp_udp_bottleneck --bw=2Mbps --delay=20ms --udpRate=3Mbps --q=50"
./ns3 run "scratch/exp8_tcp_udp_bottleneck --bw=2Mbps --delay=10ms --udpRate=5Mbps --q=10"

VISUALIZATION (save as plot_exp8.py)
import pandas as pd
import matplotlib.pyplot as plt
df = pd.read_csv("results_exp8.csv")
for metric, ylabel in [("ThroughputMbps", "Throughput (Mbps)"), ("DelayMs", "Average delay (ms)"), ("PDRPercent", "PDR (%)"), ("LossPercent", "Packet loss (%)")]:
    df.plot(kind="bar", x="Flow", y=metric, legend=False)
    plt.ylabel(ylabel); plt.title(ylabel + " by flow"); plt.grid(axis="y"); plt.tight_layout(); plt.show()
*/
