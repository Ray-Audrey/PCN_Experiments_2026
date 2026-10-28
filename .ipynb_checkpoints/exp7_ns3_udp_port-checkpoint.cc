// EXP 7: UDP traffic and port-based application delivery (NS-3.42)
#include <fstream>
#include <iostream>
#include <string>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
using namespace ns3;

int main(int argc, char *argv[]) {
    std::string delay = "2ms";
    CommandLine cmd(__FILE__);
    cmd.AddValue("delay", "Point-to-point link delay", delay);
    cmd.Parse(argc, argv);

    // 1. Nodes and topology
    NodeContainer nodes; nodes.Create(2);
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue(delay));
    NetDeviceContainer devices = p2p.Install(nodes);

    // 2. Internet stack and IP addressing
    InternetStackHelper internet; internet.Install(nodes);
    Ipv4AddressHelper ip; ip.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer iface = ip.Assign(devices);


    // 3. Applications

    // 6.1.1 Flow 1: Port 5000 (1 Mbps)

    // Receiver server on Node 1, Port 5000
    UdpServerHelper server1(5000);
    ApplicationContainer sApp1 = server1.Install(nodes.Get(1));
    sApp1.Start(Seconds(0.0));
    sApp1.Stop(Seconds(20.0));

    // Sender client on Node 0 targeting Port 5000
    UdpClientHelper client1(iface.GetAddress(1), 5000);
    client1.SetAttribute("MaxPackets", UintegerValue(0xFFFFFFFF));
    client1.SetAttribute("PacketSize", UintegerValue(512));
    client1.SetAttribute("Interval",
        TimeValue(Seconds(512 * 8.0 / 1e6)));

    ApplicationContainer cApp1 = client1.Install(nodes.Get(0));
    cApp1.Start(Seconds(1.0));
    cApp1.Stop(Seconds(20.0));


    // 6.1.2 Flow 2: Port 6000 (2 Mbps)

    // Receiver server on Node 1, Port 6000
    UdpServerHelper server2(6000);
    ApplicationContainer sApp2 = server2.Install(nodes.Get(1));
    sApp2.Start(Seconds(0.0));
    sApp2.Stop(Seconds(20.0));

    // Sender client on Node 0 targeting Port 6000
    UdpClientHelper client2(iface.GetAddress(1), 6000);
    client2.SetAttribute("MaxPackets", UintegerValue(0xFFFFFFFF));
    client2.SetAttribute("PacketSize", UintegerValue(512));
    client2.SetAttribute("Interval",
        TimeValue(Seconds(512 * 8.0 / 2e6)));

    ApplicationContainer cApp2 = client2.Install(nodes.Get(0));
    cApp2.Start(Seconds(1.0));
    cApp2.Stop(Seconds(20.0));


    // 4. Tracing and FlowMonitor
    p2p.EnablePcapAll("exp7-udp-ports");
    FlowMonitorHelper fmh; Ptr<FlowMonitor> monitor = fmh.InstallAll();
    Simulator::Stop(Seconds(20)); Simulator::Run();
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(fmh.GetClassifier());

    // 5. Metrics and CSV
    std::ofstream csv("results_exp7.csv", std::ios::app);
    if (csv.tellp() == 0) csv << "DelayMs,FlowId,Port,ThroughputMbps,DelayMsAvg,PDRPercent,LossPercent\n";
    double delayMs = std::stod(delay.substr(0, delay.find("ms")));
    for (const auto &entry : monitor->GetFlowStats()) {
        const auto &s = entry.second;
        auto tuple = classifier->FindFlow(entry.first);
        if (tuple.destinationPort != 5000 && tuple.destinationPort != 6000) continue;
        if (s.txPackets == 0) continue;
        double loss = 100.0 * (s.txPackets - s.rxPackets) / s.txPackets;
        double pdr = 100.0 * s.rxPackets / s.txPackets;
        double throughput = 0, avgDelay = 0;
        if (s.rxPackets > 0) {
            double duration = (s.timeLastRxPacket - s.timeFirstTxPacket).GetSeconds();
            if (duration > 0) throughput = s.rxBytes * 8.0 / duration / 1e6;
            avgDelay = s.delaySum.GetSeconds() * 1000.0 / s.rxPackets;
        }
        csv << delayMs << ',' << entry.first << ',' << tuple.destinationPort << ','
            << throughput << ',' << avgDelay << ',' << pdr << ',' << loss << '\n';
    }
    csv.close();
    monitor->SerializeToXmlFile("flow-results-exp7.xml", true, true);
    Simulator::Destroy();
    return 0;
}

/* RUN (from the ns-3.42 source directory; remove old CSV before a fresh sweep)
rm -f results_exp7.csv
./ns3 run "scratch/exp7_udp_ports --delay=2ms"
./ns3 run "scratch/exp7_udp_ports --delay=5ms"
./ns3 run "scratch/exp7_udp_ports --delay=10ms"
./ns3 run "scratch/exp7_udp_ports --delay=20ms"

VISUALIZATION (save as plot_exp7.py; run: python3 plot_exp7.py)
import pandas as pd
import matplotlib.pyplot as plt
df = pd.read_csv("results_exp7.csv")
for metric, ylabel in [("ThroughputMbps", "Throughput (Mbps)"), ("DelayMsAvg", "Average delay (ms)"), ("PDRPercent", "PDR (%)"), ("LossPercent", "Packet loss (%)")]:
    for port, group in df.groupby("Port"):
        plt.plot(group["DelayMs"], group[metric], marker="o", label=f"Port {port}")
    plt.xlabel("Link delay (ms)"); plt.ylabel(ylabel); plt.title(ylabel + " vs link delay")
    plt.grid(True); plt.legend(); plt.tight_layout(); plt.show()
*/
