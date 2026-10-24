
#include <fstream>
#include <iostream>
#include <string>

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/lr-wpan-module.h"
#include "ns3/sixlowpan-module.h"
#include "ns3/applications-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/propagation-module.h"

using namespace ns3;

void SamplePower(Ptr<MobilityModel> tx, Ptr<MobilityModel> rx,
                 Ptr<LogDistancePropagationLossModel> loss,
                 double txPower, double* sum, uint32_t* count)
{
    *sum += loss->CalcRxPower(txPower, tx, rx);
    ++(*count);

    if (Simulator::Now() < Seconds(19))
        Simulator::Schedule(Seconds(1), &SamplePower,
                            tx, rx, loss, txPower, sum, count);
}

int main(int argc, char* argv[])
{
    double speed = 1.0;
    CommandLine cmd(__FILE__);
    cmd.AddValue("speed", "Mobile speed in m/s", speed);
    cmd.Parse(argc, argv);

    NodeContainer sta, ap, zig;
    sta.Create(1);
    ap.Create(1);
    zig.Create(2);

    NodeContainer wifiNodes;
    wifiNodes.Add(sta);
    wifiNodes.Add(ap);

    // Wi-Fi
    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    channel.AddPropagationLoss("ns3::LogDistancePropagationLossModel");

    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    WifiHelper wifi;
    wifi.SetStandard(WIFI_STANDARD_80211n);

    WifiMacHelper mac;
    Ssid ssid("campus");

    mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid),
                "ActiveProbing", BooleanValue(false));
    NetDeviceContainer staDev = wifi.Install(phy, mac, sta);

    mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
    NetDeviceContainer apDev = wifi.Install(phy, mac, ap);

    NetDeviceContainer wifiDev;
    wifiDev.Add(staDev);
    wifiDev.Add(apDev);

    // Mobility
    MobilityHelper mobile;
    mobile.SetMobilityModel(
        "ns3::RandomWalk2dMobilityModel",
        "Bounds", RectangleValue(Rectangle(-50, 50, -50, 50)),
        "Speed", StringValue("ns3::ConstantRandomVariable[Constant=" +
                             std::to_string(speed) + "]"));
    mobile.Install(sta);
    mobile.Install(zig.Get(0));

    MobilityHelper fixed;
    fixed.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    fixed.Install(ap);
    fixed.Install(zig.Get(1));

    ap.Get(0)->GetObject<MobilityModel>()->SetPosition(Vector(15, 0, 0));
    zig.Get(0)->GetObject<MobilityModel>()->SetPosition(Vector(0, 0, 0));
    zig.Get(1)->GetObject<MobilityModel>()->SetPosition(Vector(15, 0, 0));

    // LR-WPAN + 6LoWPAN
    LrWpanHelper lr;
    NetDeviceContainer lrDev = lr.Install(zig);
    lr.CreateAssociatedPan(lrDev, 0);

    InternetStackHelper internet;
    internet.Install(wifiNodes);
    internet.Install(zig);

    SixLowPanHelper six;
    NetDeviceContainer zigDev = six.Install(lrDev);

    // IP addresses
    Ipv4AddressHelper ip4;
    ip4.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer wifiIf = ip4.Assign(wifiDev);

    Ipv6AddressHelper ip6;
    ip6.SetBase(Ipv6Address("2001:2::"), Ipv6Prefix(64));
    Ipv6InterfaceContainer zigIf = ip6.Assign(zigDev);

    // Wi-Fi UDP
    PacketSinkHelper ws("ns3::UdpSocketFactory",
        InetSocketAddress(Ipv4Address::GetAny(), 5000));
    ApplicationContainer wa = ws.Install(ap.Get(0));
    wa.Start(Seconds(0));
    wa.Stop(Seconds(20));

    OnOffHelper wc("ns3::UdpSocketFactory",
        InetSocketAddress(wifiIf.GetAddress(1), 5000));
    wc.SetConstantRate(DataRate("1Mbps"), 512);
    ApplicationContainer wb = wc.Install(sta.Get(0));
    wb.Start(Seconds(1));
    wb.Stop(Seconds(19));

    // Zigbee-style IPv6 UDP
    PacketSinkHelper zs("ns3::UdpSocketFactory",
        Inet6SocketAddress(Ipv6Address::GetAny(), 6000));
    ApplicationContainer za = zs.Install(zig.Get(1));
    za.Start(Seconds(0));
    za.Stop(Seconds(20));

    OnOffHelper zc("ns3::UdpSocketFactory",
        Inet6SocketAddress(zigIf.GetAddress(1, 1), 6000));
    zc.SetConstantRate(DataRate("50kbps"), 50);
    ApplicationContainer zb = zc.Install(zig.Get(0));
    zb.Start(Seconds(1));
    zb.Stop(Seconds(19));

    // FlowMonitor
    FlowMonitorHelper wf, zf;
    Ptr<FlowMonitor> wm = wf.Install(wifiNodes);
    Ptr<FlowMonitor> zm = zf.Install(zig);

    // Estimate received signal power (dBm)
    Ptr<LogDistancePropagationLossModel> wl =
        CreateObject<LogDistancePropagationLossModel>();
    Ptr<LogDistancePropagationLossModel> zl =
        CreateObject<LogDistancePropagationLossModel>();

    wl->SetAttribute("Exponent", DoubleValue(3.0));
    zl->SetAttribute("Exponent", DoubleValue(3.0));

    double wsum = 0, zsum = 0;
    uint32_t wn = 0, zn = 0;

    Simulator::Schedule(Seconds(1), &SamplePower,
        sta.Get(0)->GetObject<MobilityModel>(),
        ap.Get(0)->GetObject<MobilityModel>(),
        wl, 16.0, &wsum, &wn);

    Simulator::Schedule(Seconds(1), &SamplePower,
        zig.Get(0)->GetObject<MobilityModel>(),
        zig.Get(1)->GetObject<MobilityModel>(),
        zl, 0.0, &zsum, &zn);

    phy.EnablePcapAll("exp11-wifi");
    lr.EnablePcapAll("exp11-zigbee", true);

    Simulator::Stop(Seconds(20));
    Simulator::Run();

    wm->CheckForLostPackets();
    zm->CheckForLostPackets();

    double wsignal = wn ? wsum / wn : 0;
    double zsignal = zn ? zsum / zn : 0;

    std::ofstream out("./scratch/results_exp11.csv", std::ios::app);
    if (out.tellp() == 0)
        out << "Technology,Speed,SignalStrength,Throughput,PacketLoss,Delay\n";

    auto save = [&](const FlowMonitor::FlowStats& s,
                    std::string tech, double signal)
    {
        if (s.txPackets == 0) return;

        double loss = 100.0 * (s.txPackets - s.rxPackets) / s.txPackets;
        double throughput = 0, delay = 0;

        if (s.rxPackets > 0)
        {
            double duration =
                (s.timeLastRxPacket - s.timeFirstTxPacket).GetSeconds();
            if (duration > 0)
                throughput = s.rxBytes * 8.0 / duration / 1e6;
            delay = s.delaySum.GetSeconds() * 1000.0 / s.rxPackets;
        }

        out << tech << "," << speed << "," << signal << ","
            << throughput << "," << loss << "," << delay << "\n";
    };

    Ptr<Ipv4FlowClassifier> wc1 =
        DynamicCast<Ipv4FlowClassifier>(wf.GetClassifier());
    for (const auto& f : wm->GetFlowStats())
        if (wc1->FindFlow(f.first).destinationPort == 5000)
            save(f.second, "WiFi", wsignal);

    Ptr<Ipv6FlowClassifier> zc1 =
        DynamicCast<Ipv6FlowClassifier>(zf.GetClassifier6());
    for (const auto& f : zm->GetFlowStats())
        if (zc1->FindFlow(f.first).destinationPort == 6000)
            save(f.second, "Zigbee", zsignal);

    out.close();
    wm->SerializeToXmlFile("flow-wifi.xml", true, true);
    zm->SerializeToXmlFile("flow-zigbee.xml", true, true);

    Simulator::Destroy();
    return 0;
}

/*
RUN:
cd /mnt/d/ns-allinone-3.42/ns-3.42
rm -f results.csv
./ns3 run "scratch/heterogeneous_wireless --speed=1"
./ns3 run "scratch/heterogeneous_wireless --speed=5"
./ns3 run "scratch/heterogeneous_wireless --speed=10"

OUTPUT FILES:
results.csv
flow-wifi.xml
flow-zigbee.xml
exp11-wifi-*.pcap
exp11-zigbee-*.pcap

VISUALIZATION:
Save the following as plot_exp11.py in the NS-3.42 directory.

import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv("exp11_results.csv")

for metric in ["SignalStrength", "Throughput",
               "PacketLoss", "Delay"]:
    pivot = df.pivot(index="Speed",
                     columns="Technology",
                     values=metric)
    pivot.plot(kind="bar")
    plt.title(metric + " Comparison")
    plt.xlabel("Speed (m/s)")
    plt.ylabel(metric)
    plt.grid(axis="y")
    plt.tight_layout()
    plt.show()

RUN VISUALIZATION:
python3 plot_exp11.py
*/
