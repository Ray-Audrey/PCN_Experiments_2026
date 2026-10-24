#include <fstream>
#include <iostream>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

int main(int argc, char *argv[])
{
    std::string bw = "2Mbps";
    std::string delay = "10ms";
    std::string udpRate = "1Mbps";
    uint32_t q = 50;

    CommandLine cmd(__FILE__);
    cmd.AddValue("bw", "Bottleneck bandwidth", bw);
    cmd.AddValue("delay", "Bottleneck delay", delay);
    cmd.AddValue("udpRate", "UDP data rate", udpRate);
    cmd.AddValue("q", "Queue size", q);
    cmd.Parse(argc, argv);

    NodeContainer hosts;
    hosts.Create(4);

    NodeContainer routers;
    routers.Create(2);

    PointToPointHelper access, bottleneck;

    access.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    access.SetChannelAttribute("Delay", StringValue("2ms"));
    access.SetQueue("ns3::DropTailQueue<Packet>",
                    "MaxSize",
                    StringValue(std::to_string(q) + "p"));

    bottleneck.SetDeviceAttribute("DataRate", StringValue(bw));
    bottleneck.SetChannelAttribute("Delay", StringValue(delay));
    bottleneck.SetQueue("ns3::DropTailQueue<Packet>",
                        "MaxSize",
                        StringValue(std::to_string(q) + "p"));

    NetDeviceContainer d0r1 =
        access.Install(hosts.Get(0), routers.Get(0));

    NetDeviceContainer d1r1 =
        access.Install(hosts.Get(1), routers.Get(0));

    NetDeviceContainer r1r2 =
        bottleneck.Install(routers.Get(0), routers.Get(1));

    NetDeviceContainer r2n2 =
        access.Install(routers.Get(1), hosts.Get(2));

    NetDeviceContainer r2n3 =
        access.Install(routers.Get(1), hosts.Get(3));
    InternetStackHelper internet;
    internet.Install(hosts);
    internet.Install(routers);

    Ipv4AddressHelper ip;

    ip.SetBase("10.1.1.0", "255.255.255.0");
    ip.Assign(d0r1);

    ip.SetBase("10.1.2.0", "255.255.255.0");
    ip.Assign(d1r1);

    ip.SetBase("10.1.3.0", "255.255.255.0");
    ip.Assign(r1r2);

    ip.SetBase("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer i3 = ip.Assign(r2n2);

    ip.SetBase("10.1.5.0", "255.255.255.0");
    Ipv4InterfaceContainer i4 = ip.Assign(r2n3);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    uint16_t tcpPort = 5000;
    uint16_t udpPort = 5001;

    PacketSinkHelper tcpSink(
        "ns3::TcpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), tcpPort));

    ApplicationContainer tcpSinkApp =
        tcpSink.Install(hosts.Get(3));

    tcpSinkApp.Start(Seconds(0.0));
    tcpSinkApp.Stop(Seconds(20.0));

    BulkSendHelper tcpSource(
        "ns3::TcpSocketFactory",
        InetSocketAddress(i4.GetAddress(1), tcpPort));

    tcpSource.SetAttribute(
        "SendSize",
        UintegerValue(1024));

    tcpSource.SetAttribute(
        "MaxBytes",
        UintegerValue(0));

    ApplicationContainer tcpSourceApp =
        tcpSource.Install(hosts.Get(0));

    tcpSourceApp.Start(Seconds(1.0));
    tcpSourceApp.Stop(Seconds(20.0));

    PacketSinkHelper udpSink(
        "ns3::UdpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), udpPort));

    ApplicationContainer udpSinkApp =
        udpSink.Install(hosts.Get(2));

    udpSinkApp.Start(Seconds(0.0));
    udpSinkApp.Stop(Seconds(20.0));

    OnOffHelper udpSource(
        "ns3::UdpSocketFactory",
        InetSocketAddress(i3.GetAddress(1), udpPort));

    udpSource.SetConstantRate(
        DataRate(udpRate),
        512);

    ApplicationContainer udpSourceApp =
        udpSource.Install(hosts.Get(1));

    udpSourceApp.Start(Seconds(1.0));
    udpSourceApp.Stop(Seconds(20.0));

    access.EnablePcapAll("access");
    bottleneck.EnablePcapAll("bottleneck");

    FlowMonitorHelper fmh;
    Ptr<FlowMonitor> monitor = fmh.InstallAll();

    Simulator::Stop(Seconds(20.0));

    Simulator::Run();

    monitor->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(
            fmh.GetClassifier());

    std::ofstream csv("./scratch/results.csv", std::ios::app);

    if (csv.tellp() == 0)
    {
        csv << "Bandwidth,Flow,PDR(%),Loss(%),Delay(ms),"
            "Throughput(Mbps),Retransmission(%),UDPInterval(s)\n";
    }

    auto stats = monitor->GetFlowStats();

    double sumThr = 0;
    double sumThrSq = 0;
    int nFlows = 0;

    /* Change these to match your UDP settings */
    double packetSizeBytes = 512;
    double dataRateBps = 1000000.0; // 1 Mbps

    double udpInterval =
        packetSizeBytes * 8.0 / dataRateBps;

    for (const auto &flow : stats)
    {
        Ipv4FlowClassifier::FiveTuple t =
            classifier->FindFlow(flow.first);

        FlowMonitor::FlowStats s = flow.second;

        bool tcp =
            (t.protocol == 6 &&
            t.destinationPort == tcpPort);

        bool udp =
            (t.protocol == 17 &&
            t.destinationPort == udpPort);

        if (!tcp && !udp)
            continue;

        double pdr =
            s.txPackets ?
            100.0 * s.rxPackets /
            s.txPackets : 0;

        double loss =
            s.txPackets ?
            100.0 *
            (s.txPackets - s.rxPackets) /
            s.txPackets : 0;

        double delayMs =
            s.rxPackets ?
            1000.0 *
            s.delaySum.GetSeconds() /
            s.rxPackets : 0;

        double duration =
            (s.timeLastRxPacket -
            s.timeFirstTxPacket)
            .GetSeconds();

        double throughput =
            duration > 0 ?
            s.rxBytes * 8.0 /
            duration / 1e6 : 0;

        double retransRate =
            (tcp && s.txPackets) ?
            100.0 *
            (s.txPackets - s.rxPackets) /
            s.txPackets : 0;

        sumThr += throughput;
        sumThrSq += throughput * throughput;
        nFlows++;

        std::cout << "\n=============================\n";
        std::cout << (tcp ? "TCP Flow" : "UDP Flow")
                << "\n";
        std::cout << "PDR              : "
                << pdr << " %\n";
        std::cout << "Packet Loss      : "
                << loss << " %\n";
        std::cout << "Average Delay    : "
                << delayMs << " ms\n";
        std::cout << "Throughput       : "
                << throughput << " Mbps\n";

        if (tcp)
        {
            std::cout
                << "Retransmission Rate : "
                << retransRate << " %\n";
        }

        if (udp)
        {
            std::cout
                << "UDP Interval     : "
                << udpInterval << " s\n";
        }

        csv << bw << ","
            << (tcp ? "TCP" : "UDP") << ","
            << pdr << ","
            << loss << ","
            << delayMs << ","
            << throughput << ","
            << (tcp ? retransRate : 0) << ","
            << (udp ? udpInterval : 0)
            << "\n";
    }

    double fairness =
        (nFlows > 0 && sumThrSq > 0)
        ? (sumThr * sumThr) /
        (nFlows * sumThrSq)
        : 0;

    std::cout << "\n=============================\n";
    std::cout << "Jain Fairness Index : "
            << fairness << "\n";

    csv << bw
        << ",Fairness,,,,,,"
        << fairness << "\n";

    csv.close();

    Simulator::Destroy();
    return 0;
}