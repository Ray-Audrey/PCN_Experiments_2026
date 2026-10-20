
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"

using namespace ns3;

double signalSum = 0.0;
uint32_t signalCount = 0;

void ReceiveSignal(Ptr<const Packet> packet,
                   uint16_t frequency,
                   WifiTxVector txVector,
                   MpduInfo mpdu,
                   SignalNoiseDbm signalNoise,
                   uint16_t staId)
{
    signalSum += signalNoise.signal;
    signalCount++;
}

void RunScenario(double speed)
{
    signalSum = 0.0;
    signalCount = 0;

    double simTime = 10.0;

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

    YansWifiChannelHelper channel =
        YansWifiChannelHelper::Default();

    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    Ssid ssid = Ssid("campus-wifi");
    WifiMacHelper mac;

    mac.SetType("ns3::StaWifiMac",
                "Ssid", SsidValue(ssid),
                "ActiveProbing", BooleanValue(false));

    NetDeviceContainer staDevices =
        wifi.Install(phy, mac, stations);

    mac.SetType("ns3::ApWifiMac",
                "Ssid", SsidValue(ssid));

    NetDeviceContainer apDevice =
        wifi.Install(phy, mac, ap);

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
            "ns3::ConstantRandomVariable[Constant=" +
            std::to_string(speed) + "]"));

    mobility.Install(stations);

    MobilityHelper apMobility;
    apMobility.SetMobilityModel(
        "ns3::ConstantPositionMobilityModel");
    apMobility.Install(ap);

    InternetStackHelper internet;
    internet.Install(stations);
    internet.Install(ap);

    NetDeviceContainer allDevices;
    allDevices.Add(staDevices);
    allDevices.Add(apDevice);

    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");

    Ipv4InterfaceContainer interfaces =
        address.Assign(allDevices);

    uint16_t port = 9;

    PacketSinkHelper sink(
        "ns3::UdpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), port));

    ApplicationContainer sinkApp = sink.Install(ap.Get(0));
    sinkApp.Start(Seconds(0.0));
    sinkApp.Stop(Seconds(simTime));

    OnOffHelper source(
        "ns3::UdpSocketFactory",
        InetSocketAddress(interfaces.GetAddress(3), port));

    source.SetAttribute("DataRate", StringValue("1Mbps"));
    source.SetAttribute("PacketSize", UintegerValue(512));
    source.SetAttribute("OnTime", StringValue(
        "ns3::ConstantRandomVariable[Constant=1]"));
    source.SetAttribute("OffTime", StringValue(
        "ns3::ConstantRandomVariable[Constant=0]"));

    ApplicationContainer sourceApp =
        source.Install(stations.Get(0));

    sourceApp.Start(Seconds(1.0));
    sourceApp.Stop(Seconds(9.0));

    Config::ConnectWithoutContext(
        "/NodeList/*/DeviceList/*/$ns3::WifiNetDevice/Phy/MonitorSnifferRx",
        MakeCallback(&ReceiveSignal));

    phy.EnablePcap("assignment11-wifi", staDevices.Get(0));

    FlowMonitorHelper flowmon;
    Ptr<FlowMonitor> monitor = flowmon.InstallAll();

    Simulator::Stop(Seconds(simTime));
    Simulator::Run();

    monitor->CheckForLostPackets();

    Ptr<Ipv4FlowClassifier> classifier =
        DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());

    auto stats = monitor->GetFlowStats();

    std::cout << "\n====================================\n";
    std::cout << "Wi-Fi mobility speed: " << speed << " m/s\n";

    if (signalCount > 0)
    {
        std::cout << "Average received signal power: "
                  << signalSum / signalCount << " dBm\n";
    }
    else
    {
        std::cout << "Signal power: No MonitorSnifferRx samples\n";
    }

    for (auto it = stats.begin(); it != stats.end(); ++it)
    {
        Ipv4FlowClassifier::FiveTuple tuple =
            classifier->FindFlow(it->first);

        if (tuple.protocol != 17 ||
            tuple.destinationPort != port)
        {
            continue;
        }

        double throughput = it->second.rxBytes * 8.0 /
                            (8.0 * 1000000.0);

        double packetLoss = 0.0;

        if (it->second.txPackets > 0)
        {
            packetLoss =
                100.0 * (it->second.txPackets -
                         it->second.rxPackets) /
                it->second.txPackets;
        }

        double delay = 0.0;

        if (it->second.rxPackets > 0)
        {
            delay = it->second.delaySum.GetSeconds() /
                    it->second.rxPackets;
        }

        std::cout << "Tx packets: " << it->second.txPackets
                  << "\nRx packets: " << it->second.rxPackets
                  << "\nPacket loss: " << packetLoss << "%"
                  << "\nThroughput: " << throughput << " Mbps"
                  << "\nAverage end-to-end delay: "
                  << delay << " seconds\n";
    }

    Simulator::Destroy();
}

int main(int argc, char *argv[])
{
    RunScenario(1.0);
    RunScenario(5.0);
    RunScenario(10.0);

    return 0;
}
