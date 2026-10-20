
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

int main(int argc, char *argv[])
{
    double simulationTime = 10.0;
    double bottleneckRate = 2.0;
    uint32_t queueSize = 20;

    NodeContainer nodes;
    nodes.Create(6);

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer d02 = p2p.Install(nodes.Get(0), nodes.Get(2));
    NetDeviceContainer d12 = p2p.Install(nodes.Get(1), nodes.Get(2));

    PointToPointHelper bottleneck;
    bottleneck.SetDeviceAttribute(
        "DataRate", StringValue(std::to_string(bottleneckRate) + "Mbps"));
    bottleneck.SetChannelAttribute("Delay", StringValue("10ms"));
    bottleneck.SetQueue("ns3::DropTailQueue<Packet>",
                        "MaxSize", StringValue(std::to_string(queueSize) + "p"));

    NetDeviceContainer d23 = bottleneck.Install(nodes.Get(2), nodes.Get(3));

    NetDeviceContainer d34 = p2p.Install(nodes.Get(3), nodes.Get(4));
    NetDeviceContainer d35 = p2p.Install(nodes.Get(3), nodes.Get(5));

    InternetStackHelper internet;
    internet.Install(nodes);

    Ipv4AddressHelper address;

    address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer i02 = address.Assign(d02);

    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer i12 = address.Assign(d12);

    address.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer i23 = address.Assign(d23);

    address.SetBase("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer i34 = address.Assign(d34);

    address.SetBase("10.1.5.0", "255.255.255.0");
    Ipv4InterfaceContainer i35 = address.Assign(d35);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    uint16_t tcpPort = 50000;
    uint16_t udpPort = 50001;

    PacketSinkHelper tcpSink(
        "ns3::TcpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), tcpPort));

    ApplicationContainer tcpServer = tcpSink.Install(nodes.Get(5));
    tcpServer.Start(Seconds(0.0));
    tcpServer.Stop(Seconds(simulationTime));

    BulkSendHelper tcpClient(
        "ns3::TcpSocketFactory",
        InetSocketAddress(i35.GetAddress(1), tcpPort));

    tcpClient.SetAttribute("MaxBytes", UintegerValue(0));

    ApplicationContainer tcpApp = tcpClient.Install(nodes.Get(0));
    tcpApp.Start(Seconds(1.0));
    tcpApp.Stop(Seconds(simulationTime));

    PacketSinkHelper udpSink(
        "ns3::UdpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), udpPort));

    ApplicationContainer udpServer = udpSink.Install(nodes.Get(4));
    udpServer.Start(Seconds(0.0));
    udpServer.Stop(Seconds(simulationTime));

    OnOffHelper udpClient(
        "ns3::UdpSocketFactory",
        InetSocketAddress(i34.GetAddress(1), udpPort));

    udpClient.SetAttribute("DataRate", StringValue("1Mbps"));
    udpClient.SetAttribute("PacketSize", UintegerValue(1024));
    udpClient.SetAttribute("OnTime",
        StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    udpClient.SetAttribute("OffTime",
        StringValue("ns3::ConstantRandomVariable[Constant=0]"));

    ApplicationContainer udpApp = udpClient.Install(nodes.Get(1));
    udpApp.Start(Seconds(1.0));
    udpApp.Stop(Seconds(simulationTime));

    bottleneck.EnablePcapAll("wired-tcp-udp");

    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    Simulator::Stop(Seconds(simulationTime));
    Simulator::Run();

    monitor->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());

    std::cout << "\nWired TCP/UDP Performance\n";

    auto stats = monitor->GetFlowStats();

    for (auto it = stats.begin(); it != stats.end(); ++it)
    {
        Ipv4FlowClassifier::FiveTuple tuple =
            classifier->FindFlow(it->first);

        double throughput = it->second.rxBytes * 8.0 /
                            ((simulationTime - 1.0) * 1000000.0);

        double avgDelay = 0.0;

        if (it->second.rxPackets > 0)
        {
            avgDelay = it->second.delaySum.GetSeconds() /
                       it->second.rxPackets;
        }

        std::cout << "\nFlow " << it->first
                  << " | Protocol: " << (int)tuple.protocol
                  << " | " << tuple.sourceAddress
                  << " -> " << tuple.destinationAddress
                  << "\nTx Packets: " << it->second.txPackets
                  << "\nRx Packets: " << it->second.rxPackets
                  << "\nLost Packets: " << it->second.lostPackets
                  << "\nThroughput: " << throughput << " Mbps"
                  << "\nAverage Delay: " << avgDelay << " seconds\n";
    }

    Simulator::Destroy();
    return 0;
}
