// EXP 10: Routing protocol comparison under normal traffic, congestion and link failure (NS-3.42)
#include <fstream>
#include <iostream>
#include <string>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/rip-helper.h"
#include "ns3/olsr-helper.h"
#include "ns3/aodv-helper.h"
using namespace ns3;

int main(int argc, char *argv[]) {
    std::string proto = "RIP"; bool congested = false, failure = false;
    CommandLine cmd(__FILE__); cmd.AddValue("proto", "RIP / OLSR / AODV", proto);
    cmd.AddValue("congested", "Add extra UDP load (0/1)", congested); cmd.AddValue("failure", "Fail central link from 10s to 15s (0/1)", failure); cmd.Parse(argc, argv);
    if (proto != "RIP" && proto != "OLSR" && proto != "AODV") { std::cerr << "proto must be RIP, OLSR, or AODV\n"; return 1; }

    // 1. Nodes and routing helper
    NodeContainer hosts; hosts.Create(4); NodeContainer routers; routers.Create(2);
    InternetStackHelper internet;
    if (proto == "RIP") { RipHelper routing; internet.SetRoutingHelper(routing); }
    else if (proto == "OLSR") { OlsrHelper routing; internet.SetRoutingHelper(routing); }
    else { AodvHelper routing; internet.SetRoutingHelper(routing); }
    internet.Install(hosts); internet.Install(routers);

    // 2. Five-link topology
    PointToPointHelper p2p; p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));
    p2p.SetQueue("ns3::DropTailQueue<Packet>", "MaxSize", StringValue("50p"));
    NetDeviceContainer h0r0 = p2p.Install(hosts.Get(0), routers.Get(0));
    NetDeviceContainer h1r0 = p2p.Install(hosts.Get(1), routers.Get(0));
    NetDeviceContainer r0r1 = p2p.Install(routers.Get(0), routers.Get(1));
    NetDeviceContainer r1h2 = p2p.Install(routers.Get(1), hosts.Get(2));
    NetDeviceContainer r1h3 = p2p.Install(routers.Get(1), hosts.Get(3));

    // 3. Unique subnet per link. Do NOT call PopulateRoutingTables here: test the selected dynamic protocol.
    Ipv4AddressHelper ip;
    ip.SetBase("10.1.1.0", "255.255.255.0"); ip.Assign(h0r0);
    ip.SetBase("10.1.2.0", "255.255.255.0"); ip.Assign(h1r0);
    ip.SetBase("10.1.3.0", "255.255.255.0"); ip.Assign(r0r1);
    ip.SetBase("10.1.4.0", "255.255.255.0"); Ipv4InterfaceContainer dst1 = ip.Assign(r1h2);
    ip.SetBase("10.1.5.0", "255.255.255.0"); Ipv4InterfaceContainer dst2 = ip.Assign(r1h3);

    // 4. Two UDP client/server flows
    UdpServerHelper server1(5000), server2(6000);
    ApplicationContainer servers = server1.Install(hosts.Get(2)); servers.Add(server2.Install(hosts.Get(3)));
    servers.Start(Seconds(0)); servers.Stop(Seconds(20));
    UdpClientHelper client1(dst1.GetAddress(1), 5000);
    client1.SetAttribute("MaxPackets", UintegerValue(0xFFFFFFFF)); client1.SetAttribute("PacketSize", UintegerValue(512));
    client1.SetAttribute("Interval", TimeValue(Seconds(512.0 * 8 / 1e6)));
    ApplicationContainer clients = client1.Install(hosts.Get(0));
    UdpClientHelper client2(dst2.GetAddress(1), 6000);
    client2.SetAttribute("MaxPackets", UintegerValue(0xFFFFFFFF)); client2.SetAttribute("PacketSize", UintegerValue(512));
    client2.SetAttribute("Interval", TimeValue(Seconds(512.0 * 8 / 1e6)));
    clients.Add(client2.Install(hosts.Get(1))); clients.Start(Seconds(1)); clients.Stop(Seconds(20));

    // 5. Optional competing traffic and central-link failure
    if (congested) {
        OnOffHelper extra("ns3::UdpSocketFactory", InetSocketAddress(dst1.GetAddress(1), 5000));
        extra.SetConstantRate(DataRate("5Mbps"), 512);
        ApplicationContainer extraApp = extra.Install(hosts.Get(1)); extraApp.Start(Seconds(1)); extraApp.Stop(Seconds(20));
    }
    if (failure) {
        Ptr<Ipv4> r0Ipv4 = routers.Get(0)->GetObject<Ipv4>();
        int32_t ifIndex = r0Ipv4->GetInterfaceForDevice(r0r1.Get(0));
        if (ifIndex >= 0) {
            Simulator::Schedule(Seconds(10), &Ipv4::SetDown, r0Ipv4, static_cast<uint32_t>(ifIndex));
            Simulator::Schedule(Seconds(15), &Ipv4::SetUp, r0Ipv4, static_cast<uint32_t>(ifIndex));
        }
    }
    p2p.EnablePcapAll("exp10-routing");
    FlowMonitorHelper fmh; Ptr<FlowMonitor> monitor = fmh.InstallAll();
    Simulator::Stop(Seconds(20)); Simulator::Run(); monitor->CheckForLostPackets();

    // 6. Average PDR, delay and throughput across monitored UDP flows
    double pdrSum = 0, delaySum = 0, tpSum = 0; uint32_t n = 0;
    for (const auto &entry : monitor->GetFlowStats()) {
        const auto &s = entry.second; if (s.txPackets == 0) continue;
        double pdr = 100.0 * s.rxPackets / s.txPackets;
        double d = 0, tp = 0;
        if (s.rxPackets > 0) {
            d = s.delaySum.GetSeconds() * 1000.0 / s.rxPackets;
            double duration = (s.timeLastRxPacket - s.timeFirstTxPacket).GetSeconds();
            if (duration > 0) tp = s.rxBytes * 8.0 / duration / 1e6;
        }
        pdrSum += pdr; delaySum += d; tpSum += tp; ++n;
    }
    std::string condition = failure ? "Failure" : (congested ? "Congestion" : "Normal");
    std::ofstream csv("results_exp10.csv", std::ios::app);
    if (csv.tellp() == 0) csv << "Protocol,Condition,AvgPDRPercent,AvgDelayMs,AvgThroughputMbps,Flows\n";
    if (n > 0) csv << proto << ',' << condition << ',' << pdrSum / n << ',' << delaySum / n << ',' << tpSum / n << ',' << n << '\n';
    csv.close(); monitor->SerializeToXmlFile("flow-results-exp10.xml", true, true);
    Simulator::Destroy(); return 0;
}

/* RUN ALL CONFIGURATIONS (results append to results_exp10.csv; delete it before a fresh sweep)
rm -f results_exp10.csv
./ns3 run "scratch/exp10_routing_compare --proto=RIP"
./ns3 run "scratch/exp10_routing_compare --proto=RIP --congested=1"
./ns3 run "scratch/exp10_routing_compare --proto=RIP --failure=1"
Repeat the same three commands with --proto=OLSR and --proto=AODV.

IMPORTANT: this topology has no alternate route around the central link. A central-link failure therefore tests interruption/recovery, not successful rerouting via an alternate path. Dynamic routing protocol behavior also depends on NS-3.42 helper/module support and convergence timing.

VISUALIZATION (save as plot_exp10.py)
import pandas as pd
import matplotlib.pyplot as plt
df = pd.read_csv("results_exp10.csv")
for metric, ylabel in [("AvgPDRPercent", "Average PDR (%)"), ("AvgDelayMs", "Average delay (ms)"), ("AvgThroughputMbps", "Average throughput (Mbps)")]:
    pivot = df.pivot(index="Condition", columns="Protocol", values=metric)
    pivot.plot(kind="bar"); plt.ylabel(ylabel); plt.title(ylabel + " by routing condition")
    plt.grid(axis="y"); plt.tight_layout(); plt.show()
*/
