
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

void RunSimulation(std::string tcpType)
{
    double simulationTime = 10.0;

    Config::SetDefault(
        "ns3::TcpL4Protocol::SocketType",
        StringValue(tcpType));

    NodeContainer nodes;
    nodes.Create(4);

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
    p2p.SetChannelAttribute("Delay", StringValue("2ms"));

    NetDeviceContainer d01 = p2p.Install(nodes.Get(0), nodes.Get(1));

    PointToPointHelper bottleneck;
    bottleneck.SetDeviceAttribute("DataRate", StringValue("2Mbps"));
    bottleneck.SetChannelAttribute("Delay", StringValue("10ms"));
    bottleneck.SetQueue("ns3::DropTailQueue<Packet>",
                        "MaxSize", StringValue("20p"));

    NetDeviceContainer d12 =
        bottleneck.Install(nodes.Get(1), nodes.Get(2));

    NetDeviceContainer d23 = p2p.Install(nodes.Get(2), nodes.Get(3));

    InternetStackHelper internet;
    internet.Install(nodes);

    Ipv4AddressHelper address;

    address.SetBase("10.1.1.0", "255.255.255.0");
    address.Assign(d01);

    address.SetBase("10.1.2.0", "255.255.255.0");
    address.Assign(d12);

    address.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer i23 = address.Assign(d23);

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();

    uint16_t port = 50000;

    PacketSinkHelper sink(
        "ns3::TcpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), port));

    ApplicationContainer sinkApp = sink.Install(nodes.Get(3));
    sinkApp.Start(Seconds(0.0));
    sinkApp.Stop(Seconds(simulationTime));

    BulkSendHelper source(
        "ns3::TcpSocketFactory",
        InetSocketAddress(i23.GetAddress(1), port));

    source.SetAttribute("MaxBytes", UintegerValue(0));

    ApplicationContainer sourceApp = source.Install(nodes.Get(0));
    sourceApp.Start(Seconds(1.0));
    sourceApp.Stop(Seconds(9.0));

    bottleneck.EnablePcapAll("tcp-comparison");

    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    Simulator::Stop(Seconds(simulationTime));
    Simulator::Run();

    monitor->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());

    auto stats = monitor->GetFlowStats();

    std::cout << "\nTCP Algorithm: " << tcpType << "\n";

    for (auto it = stats.begin(); it != stats.end(); ++it)
    {
        Ipv4FlowClassifier::FiveTuple tuple =
            classifier->FindFlow(it->first);

        if (tuple.protocol == 6 && tuple.destinationPort == port)
        {
            double throughput = it->second.rxBytes * 8.0 /
                                (8.0 * 1000000.0);

            double avgDelay = 0.0;

            if (it->second.rxPackets > 0)
            {
                avgDelay = it->second.delaySum.GetSeconds() /
                           it->second.rxPackets;
            }

            std::cout << "Tx Packets: " << it->second.txPackets
                      << "\nRx Packets: " << it->second.rxPackets
                      << "\nLost Packets: " << it->second.lostPackets
                      << "\nThroughput: " << throughput << " Mbps"
                      << "\nAverage Delay: " << avgDelay
                      << " seconds\n";
        }
    }

    Simulator::Destroy();
}

int main(int argc, char *argv[])
{
    RunSimulation("ns3::TcpNewReno");
    RunSimulation("ns3::TcpCubic");
    RunSimulation("ns3::TcpBbr");

    return 0;
}
