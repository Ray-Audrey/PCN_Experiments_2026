
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

int main(int argc, char *argv[])
{
    double simulationTime = 10.0;
    double stationSpeed = 2.0;

    NodeContainer stations;
    stations.Create(3);

    NodeContainer ap;
    ap.Create(1);

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211g);

    wifi.SetRemoteStationManager(
        "ns3::ConstantRateWifiManager",
        "DataMode", StringValue("ErpOfdmRate24Mbps"),
        "ControlMode", StringValue("ErpOfdmRate6Mbps"));

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();

    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    Ssid ssid = Ssid("pcn-wifi");
    WifiMacHelper mac;

    mac.SetType("ns3::StaWifiMac",
                "Ssid", SsidValue(ssid),
                "ActiveProbing", BooleanValue(false));

    NetDeviceContainer stationDevices = wifi.Install(
        phy, mac, stations);

    mac.SetType("ns3::ApWifiMac",
                "Ssid", SsidValue(ssid));

    NetDeviceContainer apDevice = wifi.Install(
        phy, mac, ap);

    MobilityHelper mobility;

    mobility.SetPositionAllocator(
        "ns3::GridPositionAllocator",
        "MinX", DoubleValue(0.0),
        "MinY", DoubleValue(0.0),
        "DeltaX", DoubleValue(10.0),
        "DeltaY", DoubleValue(10.0),
        "GridWidth", UintegerValue(3),
        "LayoutType", StringValue("RowFirst"));

    mobility.SetMobilityModel(
        "ns3::RandomWalk2dMobilityModel",
        "Bounds", RectangleValue(Rectangle(-50, 50, -50, 50)),
        "Speed", StringValue(
            "ns3::ConstantRandomVariable[Constant=2.0]"));

    mobility.Install(stations);

    MobilityHelper apMobility;
    apMobility.SetMobilityModel(
        "ns3::ConstantPositionMobilityModel");
    apMobility.Install(ap);

    InternetStackHelper internet;
    internet.Install(stations);
    internet.Install(ap);

    NetDeviceContainer allDevices;
    allDevices.Add(stationDevices);
    allDevices.Add(apDevice);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");

    Ipv4InterfaceContainer interfaces =
        address.Assign(allDevices);

    uint16_t port = 9;

    PacketSinkHelper server(
        "ns3::UdpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), port));

    ApplicationContainer serverApp = server.Install(ap.Get(0));
    serverApp.Start(Seconds(0.0));
    serverApp.Stop(Seconds(simulationTime));

    OnOffHelper client(
        "ns3::UdpSocketFactory",
        InetSocketAddress(interfaces.GetAddress(3), port));

    client.SetAttribute("DataRate", StringValue("1Mbps"));
    client.SetAttribute("PacketSize", UintegerValue(512));
    client.SetAttribute("OnTime",
        StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    client.SetAttribute("OffTime",
        StringValue("ns3::ConstantRandomVariable[Constant=0]"));

    ApplicationContainer clientApp = client.Install(stations.Get(0));
    clientApp.Start(Seconds(1.0));
    clientApp.Stop(Seconds(simulationTime));

    phy.EnablePcap("wifi-mobility", stationDevices.Get(0));

    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    Simulator::Stop(Seconds(simulationTime));
    Simulator::Run();

    monitor->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());

    std::cout << "\nWi-Fi Mobility Performance\n";

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
                  << "\n" << tuple.sourceAddress
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
