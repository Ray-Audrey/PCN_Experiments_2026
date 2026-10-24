
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include <fstream>
using namespace ns3;

int main(int argc, char *argv[])
{
    std::string tcp = "ns3::TcpNewReno";
    CommandLine cmd(__FILE__);
    cmd.AddValue("tcpType", "TCP variant", tcp);
    cmd.Parse(argc, argv);

    Config::SetDefault("ns3::TcpL4Protocol::SocketType",
                       TypeIdValue(TypeId::LookupByName(tcp)));

    NodeContainer senders, router, server;
    senders.Create(5);
    router.Create(1);
    server.Create(1);

    PointToPointHelper access, bottleneck;
    access.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    access.SetChannelAttribute("Delay", StringValue("2ms"));
    bottleneck.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    bottleneck.SetChannelAttribute("Delay", StringValue("10ms"));
    bottleneck.SetQueue("ns3::DropTailQueue<Packet>",
                        "MaxSize", StringValue("20p"));

    NetDeviceContainer dev[5];
    for (int i = 0; i < 5; i++)
        dev[i] = access.Install(senders.Get(i), router.Get(0));

    NetDeviceContainer bottleneckDev =
        bottleneck.Install(router.Get(0), server.Get(0));

    InternetStackHelper internet;
    internet.Install(senders);
    internet.Install(router);
    internet.Install(server);

    Ipv4AddressHelper ip;
    for (int i = 0; i < 5; i++) {
        std::string subnet = "10.1." + std::to_string(i + 1) + ".0";
        ip.SetBase(subnet.c_str(), "255.255.255.0");
        ip.Assign(dev[i]);
    }
    ip.SetBase("10.1.6.0", "255.255.255.0");
    Ipv4InterfaceContainer serverIf = ip.Assign(bottleneckDev);
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    PacketSinkHelper sink("ns3::TcpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), 8080));
    sink.Install(server.Get(0)).Start(Seconds(0));

    for (int i = 0; i < 5; i++) {
        BulkSendHelper source("ns3::TcpSocketFactory",
            InetSocketAddress(serverIf.GetAddress(1), 8080));
        source.SetAttribute("MaxBytes", UintegerValue(0));
        ApplicationContainer app = source.Install(senders.Get(i));
        app.Start(Seconds(1));
        app.Stop(Seconds(19));
    }

    FlowMonitorHelper fm;
    Ptr<FlowMonitor> monitor = fm.InstallAll();
    Simulator::Stop(Seconds(20));
    Simulator::Run();
    monitor->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(fm.GetClassifier());

    double tpSum = 0, tpSq = 0, delaySum = 0, lossSum = 0;
    int n = 0;

    for (auto &f : monitor->GetFlowStats()) {
        auto s = f.second;
        auto t = classifier->FindFlow(f.first);
        if (t.destinationPort != 8080 || !s.rxPackets || !s.txPackets)
            continue;

        double duration =
            (s.timeLastRxPacket - s.timeFirstTxPacket).GetSeconds();
        if (duration <= 0) continue;

        double tp = s.rxBytes * 8.0 / duration / 1e6;
        tpSum += tp;
        tpSq += tp * tp;
        delaySum += s.delaySum.GetSeconds() * 1000 / s.rxPackets;
        lossSum += 100.0 * (s.txPackets - s.rxPackets) / s.txPackets;
        n++;
    }

    std::string label = tcp;
    if (tcp == "ns3::TcpNewReno") label = "Reno";
    else if (tcp == "ns3::TcpCubic") label = "Cubic";
    else if (tcp == "ns3::TcpBbr") label = "BBR";

    std::ofstream out("./scratch/results_exp9.csv");
    out << "TCP,ThroughputMbps,DelayMs,LossPercent,Fairness\n";
    if (n) {
        double fairness = tpSq ? tpSum * tpSum / (n * tpSq) : 0;
        out << label << "," << tpSum/n << "," << delaySum/n
            << "," << lossSum/n << "," << fairness << "\n";

        std::cout << "TCP: " << label
                  << "\nThroughput: " << tpSum/n << " Mbps"
                  << "\nDelay: " << delaySum/n << " ms"
                  << "\nPacket loss: " << lossSum/n << " %"
                  << "\nJain fairness: " << fairness << "\n";
    } else {
        std::cout << "No received TCP flows.\n";
    }

    out.close();
    monitor->SerializeToXmlFile("flow-results-exp9.xml", true, true);
    Simulator::Destroy();
    return 0;
}

/*
RUN (rename each CSV before the next run)

./ns3 run "scratch/vanet_tcp --tcpType=ns3::TcpNewReno"
mv results_exp9.csv reno.csv

./ns3 run "scratch/vanet_tcp --tcpType=ns3::TcpCubic"
mv results_exp9.csv cubic.csv

./ns3 run "scratch/vanet_tcp --tcpType=ns3::TcpBbr"
mv results_exp9.csv bbr.csv

PLOT METRICS (Python)

import pandas as pd
import matplotlib.pyplot as plt

df = pd.concat([pd.read_csv("reno.csv"),
                pd.read_csv("cubic.csv"),
                pd.read_csv("bbr.csv")], ignore_index=True)

for col, title in [
    ("ThroughputMbps", "Throughput (Mbps)"),
    ("DelayMs", "Average Delay (ms)"),
    ("LossPercent", "Packet Loss (%)"),
    ("Fairness", "Jain Fairness Index")
]:
    df.plot.bar(x="TCP", y=col, legend=False)
    plt.title(title)
    plt.grid(axis="y")
    plt.tight_layout()
    plt.show()
*/
