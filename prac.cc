#include "ns3/core-module.h" //Time,Simulator,
#include "ns3/network-module.h" //NodeContainer,Packet,
#include <fstream>
#include <iostream>
#include <string>
#include "ns3/internet-module.h" //IP,TCP,UDP,Routing
#include "ns3/point-to-point-module.h" //P2Plinksanddrop-tail
#include "ns3/applications-module.h" //UdpClient,BulkSend,
#include "ns3/flow-monitor-module.h" //FlowClassifier,FlowStats
#include "ns3/traffic-control-module.h" //QueueDiscs(FQ-CoDel,FIFO)
#include "ns3/wifi-module.h" //802.11PHYandMAC
#include "ns3/mobility-module.h" //Positionandvelocity
#include "ns3/lr-wpan-module.h" //802.15.4ZigbeePHY&
#include "ns3/sixlowpan-module.h" //IPv6encapsulationover
#include "ns3/internet-apps-module.h" //Pinganddynamic RIP
using namespace ns3;

int main(int argc,char *argv[]){
    std::string delay="2ms";
    CommandLine cmd(__FILE__);
    cmd.AddValue("delay","Point-to-Point Link Delay",delay);
    cmd.Parse(argc,argv);
    NodeContainer nodes;
    nodes.Create(2);
    PointToPointHelper p2p;
    p2p.SetDeviceAttribute("DataRate",StringValue("10Mbps"));
    p2p.SetChannelAttribute("Delay",StringValue(delay));

    NetDeviceContainer devices= p2p.Install(nodes);
    InternetStackHelper internet;
    internet.Install(nodes);
    Ipv4AddressHelper ipv4;
    ipv4.SetBase("10.1.1.0","255.255.255.0");
    Ipv4InterfaceContainer iface=ipv4.Assign(devices);
    //ReceiverServeronNode2,Port5000
    UdpServerHelper server1(5000);
    ApplicationContainer sApp1= server1.Install(nodes.Get(1));
    sApp1.Start(Seconds(0.0));
    sApp1.Stop(Seconds(20.0));
    UdpClientHelper client1(iface.GetAddress(1),5000);
    client1.SetAttribute("MaxPackets",UintegerValue(0xFFFFFFFF)); 
    client1.SetAttribute("PacketSize",UintegerValue(512));
    client1.SetAttribute("Interval",TimeValue(Seconds(512*8.0/ 1e6)));
    ApplicationContainer cApp1= client1.Install(nodes.Get(0));
    cApp1.Start(Seconds(1.0));
    cApp1.Stop(Seconds(20.0));
    //ReceiverServeronNode2,Port6000
    UdpServerHelper server2(6000);
    ApplicationContainer sApp2= server2.Install(nodes.Get(1));
    sApp2.Start(Seconds(0.0));
    sApp2.Stop(Seconds(20.0));
    //SenderClientonNode1targetingPort6000at2Mbps
    UdpClientHelper client2(iface.GetAddress(1),6000);
    client2.SetAttribute("MaxPackets",UintegerValue(0xFFFFFFFF));
    client2.SetAttribute("PacketSize",UintegerValue(512));
    client2.SetAttribute("Interval",TimeValue(Seconds(512*8.0/ 2e6)));
    ApplicationContainer cApp2= client2.Install(nodes.Get(0));
    cApp2.Start(Seconds(1.0));
    cApp2.Stop(Seconds(20.0));
    p2p.EnablePcapAll("exp7_udp_ports");
    FlowMonitorHelper fmh;
    Ptr<FlowMonitor> monitor=fmh.InstallAll();
    Simulator::Stop(Seconds(20.0));
    Simulator::Run();
    monitor->CheckForLostPackets();
    Ptr<Ipv4FlowClassifier> classifier=DynamicCast<Ipv4FlowClassifier>(fmh.GetClassifier());
    //Csv
    std::ofstream csv("./scratch/results_exp7.csv",std::ios::app);
    if(csv.tellp()==0)csv << "DelayMs,FlowId,Port,ThroughputMbps,DelayMsAvg,PDRPercent,LossPercent\n";
    double delayMs=std::stod(delay.substr(0,delay.find("ms")));
    for (const auto &entry:monitor->GetFlowStats()){
        const auto &s=entry.second;
        auto tuple=classifier->FindFlow(entry.first);
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
        // Display on terminal
        std::cout << "\nFlow ID: " << entry.first
              << "\nPort: " << tuple.destinationPort
              << "\nThroughput: " << throughput << " Mbps"
              << "\nAverage Delay: " << avgDelay << " ms"
              << "\nPDR: " << pdr << " %"
              << "\nLoss: " << loss << " %"
              << "\n------------------------" << std::endl;
        csv << delayMs << ',' << entry.first << ',' << tuple.destinationPort << ','
            << throughput << ',' << avgDelay << ',' << pdr << ',' << loss << '\n';
        
    }
    csv.close();
    monitor->SerializeToXmlFile("flow-results-exp7.xml", true, true);
    Simulator::Destroy();
    return 0;
}
