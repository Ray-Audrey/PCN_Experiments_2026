
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

void RunExperiment(double latencyMs, bool twoFlows)
{
    double simulationTime = 10.0;
    double rateMbps = 1.0;
    uint32_t packetSize = 1024;

    uint16_t port1 = 5000;
    uint16_t port2 = 6000;

    NodeContainer nodes;
    nodes.Create(2);

    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
    p2p.SetChannelAttribute(
        "Delay", StringValue(std::to_string(latencyMs) + "ms"));

    NetDeviceContainer devices = p2p.Install(nodes);

    InternetStackHelper internet;
    internet.Install(nodes);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");

    Ipv4InterfaceContainer interfaces = address.Assign(devices);

    // UDP Server 1 on Node 2
    UdpServerHelper server1(port1);
    ApplicationContainer serverApp1 = server1.Install(nodes.Get(1));

    serverApp1.Start(Seconds(0.0));
    serverApp1.Stop(Seconds(simulationTime));

    // UDP Client 1 on Node 1
    UdpClientHelper client1(interfaces.GetAddress(1), port1);

    client1.SetAttribute("MaxPackets", UintegerValue(1000000));
    client1.SetAttribute("PacketSize", UintegerValue(packetSize));
    client1.SetAttribute(
        "Interval",
        TimeValue(Seconds(packetSize * 8.0 /
                          (rateMbps * 1000000.0))));

    ApplicationContainer clientApp1 = client1.Install(nodes.Get(0));
    clientApp1.Start(Seconds(1.0));
    clientApp1.Stop(Seconds(9.0));

    // UDP Server 2 and Client 2 on a different port
    if (twoFlows)
    {
        UdpServerHelper server2(port2);
        ApplicationContainer serverApp2 = server2.Install(nodes.Get(1));

        serverApp2.Start(Seconds(0.0));
        serverApp2.Stop(Seconds(simulationTime));

        UdpClientHelper client2(interfaces.GetAddress(1), port2);

        client2.SetAttribute("MaxPackets", UintegerValue(1000000));
        client2.SetAttribute("PacketSize", UintegerValue(packetSize));
        client2.SetAttribute(
            "Interval",
            TimeValue(Seconds(packetSize * 8.0 /
                              (rateMbps * 1000000.0))));

        ApplicationContainer clientApp2 = client2.Install(nodes.Get(0));
        clientApp2.Start(Seconds(1.0));
        clientApp2.Stop(Seconds(9.0));
    }

    // Unique PCAP file prefix for each experiment
    std::string prefix = "udp-" + std::to_string((int)latencyMs) + "ms";
    if (twoFlows)
    {
        prefix += "-two-flows";
    }
    else
    {
        prefix += "-one-flow";
    }

    p2p.EnablePcapAll(prefix);

    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    Simulator::Stop(Seconds(simulationTime));
    Simulator::Run();

    monitor->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());

    auto stats = monitor->GetFlowStats();

    double throughput1 = 0.0;
    double throughput2 = 0.0;

    // Clients send from 1s to 9s: measurement interval = 8 seconds
    for (auto it = stats.begin(); it != stats.end(); ++it)
    {
        Ipv4FlowClassifier::FiveTuple tuple =
            classifier->FindFlow(it->first);

        if (tuple.destinationPort == port1)
        {
            throughput1 = it->second.rxBytes * 8.0 /
                          (8.0 * 1000000.0);
        }
        else if (tuple.destinationPort == port2)
        {
            throughput2 = it->second.rxBytes * 8.0 /
                          (8.0 * 1000000.0);
        }
    }

    std::cout << latencyMs << " ms\t"
              << throughput1 << " Mbps\t";

    if (twoFlows)
    {
        std::cout << throughput2 << " Mbps";
    }
    else
    {
        std::cout << "Not active";
    }

    std::cout << std::endl;

    Simulator::Destroy();
}

int main(int argc, char *argv[])
{
    double latencies[] = {2, 5, 10, 20};

    std::cout << "\nONE UDP FLOW\n";
    std::cout << "Latency\tFlow 1 Throughput\n";

    for (double latency : latencies)
    {
        RunExperiment(latency, false);
    }

    std::cout << "\nTWO UDP FLOWS\n";
    std::cout << "Latency\tFlow 1 Throughput\tFlow 2 Throughput\n";

    for (double latency : latencies)
    {
        RunExperiment(latency, true);
    }

    return 0;
}
