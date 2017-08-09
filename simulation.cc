/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
#include "ns3/aodv-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h" 
#include "ns3/v4ping-helper.h"
#include "ns3/ipv4.h"

#include <iostream>
#include <fstream>
#include <sstream>

#include "ns3/ns2-mobility-helper.h"

#include "ns3/tracker-helper.h"

#include "ns3/netanim-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("Simulation");

// For mobility tracing:
// Prints actual position and velocity when a course change event occurs
static void
CourseChange (std::ostream *os, std::string foo, Ptr<const MobilityModel> mobility)
{
    Vector pos = mobility->GetPosition (); // Get position
    Vector vel = mobility->GetVelocity (); // Get velocity

    // Prints position and velocities
    *os << Simulator::Now () << " POS: x=" << pos.x << ", y=" << pos.y
        << ", z=" << pos.z << "; VEL:" << vel.x << ", y=" << vel.y
        << ", z=" << vel.z << std::endl;
}

int main(int argc, char *argv[])
{
    LogComponentEnable ("Simulation", LOG_LEVEL_INFO); // Comment this to disable infos logging.

    //------------------------------------------------------------------------------------------------------------------
    // Initialisation:
    //------------------------------------------------------------------------------------------------------------------
    NS_LOG_INFO ("Initialisation...");

    uint32_t nodeNum (20);
    double step (100);
    double totalTime (120);
    bool pcap (false);
    bool printRoutes (false);
    int t; // For simulation time calculation.
    std::string traceFile; // For sumo mobility.
    std::string logFile; // For sumo mobility.

    std::ofstream os;

    NodeContainer nodes;
    NetDeviceContainer devices;
    Ipv4InterfaceContainer interfaces;

    //------------------------------------------------------------------------------------------------------------------
    // Configuration:
    //------------------------------------------------------------------------------------------------------------------
    NS_LOG_INFO ("Configuration...");

    //LogComponentEnable("AodvRoutingProtocol", LOG_LEVEL_ALL); // Comment this if too noisy.
    SeedManager::SetSeed (1234567);
    CommandLine cmd;
    cmd.AddValue ("pcap", "Write PCAP traces.", pcap);
    cmd.AddValue ("printRoutes", "Print routing table dumps.", printRoutes);
    cmd.AddValue ("nodeNum", "Number of nodes.", nodeNum);
    cmd.AddValue ("time", "Simulation time, s.", totalTime);
    cmd.AddValue ("step", "Grid step, m", step);
    cmd.AddValue ("traceFile", "Ns2 movement trace file", traceFile);
    cmd.AddValue ("logFile", "Log file", logFile);
    cmd.Parse (argc, argv);

    //------------------------------------------------------------------------------------------------------------------
    // Execution:
    //------------------------------------------------------------------------------------------------------------------
    NS_LOG_INFO ("Execution...");

    Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", UintegerValue (1)); // enable rts cts all the time.
	
    //----------------------------------------------------------------
    // Create  nodes:
    //----------------------------------------------------------------
    NS_LOG_INFO ("-> Creating nodes...");

    //std::cout << "Creating " << (unsigned)nodeNum << " nodes " << step << " m apart.\n"; // Similar as: NS_LOG_UNCOND.
    nodes.Create (nodeNum);
    // Name nodes
    for (uint32_t i = 0; i < nodeNum; ++i)
    {
        std::ostringstream os;
        os << "node-" << i;
        Names::Add (os.str (), nodes.Get (i));
    }

    //----------------------------------------------------------------
    // Setting up mobility model:
    //----------------------------------------------------------------
    NS_LOG_INFO ("-> Setting up mobility model...");

    if (traceFile.empty () || logFile.empty ())
    {
        std::cout << "\"logFile\" and \"traceFile\" are not specified... using Constant Position Mobility Model." << std::endl;

        MobilityHelper mobility;
        mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
                                        "MinX", DoubleValue (0.0),
                                        "MinY", DoubleValue (0.0),
                                        "DeltaX", DoubleValue (step),
                                        "DeltaY", DoubleValue (0),
                                        "GridWidth", UintegerValue (nodeNum),
                                        "LayoutType", StringValue ("RowFirst"));
        mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
        mobility.Install (nodes);

        for (NodeContainer::Iterator i = nodes.Begin (); i != nodes.End (); ++i)
        {

        }


    }
    else
    {
        // Enable logging from the ns2 helper
        //LogComponentEnable ("Ns2MobilityHelper",LOG_LEVEL_INFO);

        // Create Ns2MobilityHelper with the specified trace log file as parameter
        Ns2MobilityHelper ns2 = Ns2MobilityHelper (traceFile);

        // open log file for mobility output
        std::ofstream os;
        os.open (logFile.c_str ());

        ns2.Install (); // configure movements for each node, while reading trace file

        // Configure callback for logging
        Config::Connect ("/NodeList/*/$ns3::MobilityModel/CourseChange",
                            MakeBoundCallback (&CourseChange, &os));
    }

    //----------------------------------------------------------------
    // Create devices:
    //----------------------------------------------------------------
    NS_LOG_INFO ("-> Creating devices...");

    NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default ();
    wifiMac.SetType ("ns3::AdhocWifiMac");
    YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
    YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
    wifiPhy.SetChannel (wifiChannel.Create ());
    WifiHelper wifi = WifiHelper::Default ();
    wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("OfdmRate6Mbps"), "RtsCtsThreshold", UintegerValue (0));
    devices = wifi.Install (wifiPhy, wifiMac, nodes); 

/* Printing NetDevices pointers.
    for (NetDeviceContainer::Iterator i = devices.Begin (); i != devices.End (); ++i)
    {
        std::cout << *(i) << "---" << std::endl;
    }
//*/

    //*(nodes.Begin ())->GetObject<NetDevice>;

    if (pcap)
    {
        wifiPhy.EnablePcapAll (std::string ("aodv"));
    }

    //----------------------------------------------------------------
    // Install Internet Stacks:
    //----------------------------------------------------------------
    NS_LOG_INFO ("-> Installing internet stacks...");

    AodvHelper aodv;
    aodv.Set ("EnableBroadcast", BooleanValue (false));
    // you can configure AODV attributes here using aodv.Set(name, value)
    InternetStackHelper stack;
    stack.SetRoutingHelper (aodv); // has effect on the next Install ()
    stack.Install (nodes);
    Ipv4AddressHelper address;
    address.SetBase ("10.0.0.0", "255.0.0.0");
    interfaces = address.Assign (devices);

    if (printRoutes)
    {
        Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("aodv.routes", std::ios::out);
        //aodv.PrintRoutingTableAllAt (Seconds (8), routingStream);
        aodv.PrintRoutingTableAllEvery(Seconds (5), routingStream);
    }

    std::vector<aodv::Neighbors::Neighbor>  m_nb;


    //----------------------------------------------------------------
    // Install Applications:
    //----------------------------------------------------------------
    NS_LOG_INFO ("-> Installing applications...");
    
    LogComponentEnable ("Tracker", LOG_LEVEL_INFO);

    /*
    V4PingHelper ping (interfaces.GetAddress (nodeNum - 1));
    ping.SetAttribute ("Verbose", BooleanValue (true));

    ApplicationContainer p = ping.Install (nodes.Get (0));
    p.Start (Seconds (0));
    p.Stop (Seconds (totalTime) - Seconds (0.001));
    */
    /*
    TrackerHelper Server (9, interfaces.GetAddress (nodeNum-1), 9);
    Server.SetAttribute ("MaxPackets", UintegerValue (1));
    Server.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
    Server.SetAttribute ("PacketSize", UintegerValue (1024));

    ApplicationContainer serverApps = Server.Install (nodes.Get (0));
    Server.SetFill (*(serverApps.Begin ()), std::string("Bonjour!"));

    TrackerHelper Client (9, interfaces.GetAddress (0), 9);
    Client.SetAttribute ("MaxPackets", UintegerValue (1)); // 1 =nPackets
    Client.SetAttribute ("Interval", TimeValue (Seconds (1.0)));
    Client.SetAttribute ("PacketSize", UintegerValue (1024));

    ApplicationContainer clientApps = Client.Install (nodes.Get (nodeNum-1));
    Client.SetFill (*(clientApps.Begin ()), std::string("Bonjour!"));

    clientApps.Start (Seconds (0));
    serverApps.Start (Seconds (0));
    serverApps.Stop (Seconds (totalTime));
    clientApps.Stop (Seconds (totalTime));
    //*/

    // Define a broadcast address:
    //static const ns3::InetSocketAddress kBeaconBroadcast = ns3::InetSocketAddress(ns3::Ipv4Address("255.255.255.255"), /* beacon_port */ 9); 

    //std::cout << Ipv4Address ("10.255.255.255").IsSubnetDirectedBroadcast (Ipv4Mask ("255.0.0.0")) << std::endl;
    //TrackerHelper tracker (9,  Ipv4Address ("10.255.255.255") /*Ipv4Address ("10.0.0.2")*/ /*interfaces.GetAddress (nodeNum-1)*/, 9);
    TrackerHelper tracker;
    tracker.SetAttribute ("MaxPackets", UintegerValue (100));
    tracker.SetAttribute ("Interval", TimeValue (Seconds (6.0)));
    tracker.SetAttribute ("PacketSize", UintegerValue (1024));
    
    ApplicationContainer applications = tracker.Install (nodes);

    //applications.Get (2).Track ();

    applications.Start (Seconds (0));
    applications.Stop (Seconds (totalTime));

/*
    if (traceFile.empty () || logFile.empty ())
    {
        // move node away
        Ptr<Node> node = nodes.Get (nodeNum/2);
        Ptr<MobilityModel> mob = node->GetObject<MobilityModel> ();
        Simulator::Schedule (Seconds (totalTime/3), &MobilityModel::SetPosition, mob, Vector (20, -10, 0));
    }
//*/
    std::cout << "Starting simulation for " << totalTime << " s ...\n"; // Similar as: NS_LOG_UNCOND.
    t = time (0);

    AnimationInterface anim ("lm.xml"); // Write Netanime .xml trace.
    Simulator::Stop (Seconds (totalTime));
    Simulator::Run ();
    Simulator::Destroy ();

    if (! (traceFile.empty () || logFile.empty ()))
    {
        os.close (); // close mobility log file     
    }

    //------------------------------------------------------------------------------------------------------------------
    // Report:
    //------------------------------------------------------------------------------------------------------------------

    // Add some reporting if required.

    //------------------------------------------------------------------------------------------------------------------
    t = time (0) - t;
    std::cout << "Simulation took " << t << " s." << std::endl;     

    return 0;
}
