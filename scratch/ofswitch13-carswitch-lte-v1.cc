/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2014 University of Campinas (Unicamp)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Luciano Chaves <luciano@lrc.ic.unicamp.br>
 *         Vitor M. Eichemberger <vitor.marge@gmail.com>
 *
 * Two hosts connected to a single OpenFlow switch.
 * The switch is managed by the default learning controller application.
 * The connection between the switch and the controller uses LTE.
 *
 *                       Learning Controller
 *                                |
 *                       +-----------------+
 *            Host 0 === | OpenFlow switch | === Host 1
 *                       +-----------------+
 *
 * LTE example by Author: Jaume Nin <jaume.nin@cttc.cat>
 *
 * Adapted by Oscar Bautista <obaut004@fiu.edu>
 */

// OFSwith13 includes
#include <ns3/core-module.h>
#include <ns3/network-module.h>
#include <ns3/csma-module.h>
#include <ns3/internet-module.h>
#include <ns3/ofswitch13-module.h>
#include <ns3/internet-apps-module.h>
#include <ns3/traffic-type-controller.h>

 //LTE includes
 #include "ns3/lte-helper.h"
 #include "ns3/epc-helper.h"
 //#include "ns3/core-module.h"
 //#include "ns3/network-module.h"
 #include "ns3/ipv4-global-routing-helper.h"
 //#include "ns3/internet-module.h"
 #include "ns3/mobility-module.h"
 #include "ns3/lte-module.h"
 #include "ns3/applications-module.h"
 #include "ns3/point-to-point-helper.h"
 #include "ns3/config-store.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SimpleSDNthroughLTE");

int
main (int argc, char *argv[])
{
  uint16_t simTime = 6;
  bool verbose = true;
  bool trace = false;
  //Lte
  double distance = 60.0;
  bool useCa = false;

  bool v4pingVerbose = true;

  // Configure command line parameters
  CommandLine cmd;
  cmd.AddValue ("simTime", "Simulation time (seconds)", simTime);
  cmd.AddValue ("verbose", "Enable verbose output", verbose);
  cmd.AddValue ("trace", "Enable datapath stats and pcap traces", trace);
  //Lte
  cmd.AddValue("distance", "Distance between eNBs [m]", distance);
  cmd.AddValue("useCa", "Whether to use carrier aggregation.", useCa);

  cmd.AddValue ("v4ping-verbose", "turn on log component for V4Ping Application", v4pingVerbose);

  cmd.Parse (argc, argv);

  if (verbose)
    {
      OFSwitch13Helper::EnableDatapathLogs ();
      //LogComponentEnable ("OFSwitch13Interface", LOG_LEVEL_ALL);
      //LogComponentEnable ("OFSwitch13Device", LOG_LEVEL_ALL);
      LogComponentEnable ("OFSwitch13Port", LOG_LEVEL_ALL);
      //LogComponentEnable ("OFSwitch13Queue", LOG_LEVEL_ALL);
      //LogComponentEnable ("OFSwitch13SocketHandler", LOG_LEVEL_ALL);
      //LogComponentEnable ("OFSwitch13Controller", LOG_LEVEL_ALL);
      //LogComponentEnable ("OFSwitch13LearningController", LOG_LEVEL_ALL);
      //LogComponentEnable ("OFSwitch13Helper", LOG_LEVEL_ALL);
      //LogComponentEnable ("OFSwitch13InternalHelper", LOG_LEVEL_ALL);
      //LogComponentEnable ("SimpleSDNthroughLTE", LOG_LEVEL_ALL);
    }

  if (v4pingVerbose)
    {
      // LogComponentEnable ("TcpSocketBase", LOG_LEVEL_FUNCTION);
      // LogComponentEnable ("Socket", LOG_LEVEL_FUNCTION);
      // LogComponentEnable ("TcpL4Protocol", LOG_LEVEL_FUNCTION);

      LogComponentEnable ("V4Ping", LOG_LEVEL_INFO);
      // LogComponentEnable ("Ipv4RawSocketImpl", LOG_LEVEL_INFO);
      // LogComponentEnable ("Socket", LOG_LEVEL_INFO);
      // LogComponentEnable ("Ipv4L3Protocol", LOG_LEVEL_INFO);
      // LogComponentEnable ("Ipv4Interface", LOG_LEVEL_INFO);
      // LogComponentEnable ("ArpL3Protocol", LOG_LEVEL_INFO);
      // LogComponentEnable ("TrafficControlLayer", LOG_LEVEL_INFO);
      // LogComponentEnable ("QueueDisc", LOG_LEVEL_INFO);
      LogComponentEnable ("CsmaNetDevice", LOG_LEVEL_INFO);
      // LogComponentEnable ("LteUeNetDevice", LOG_LEVEL_INFO);
      // LogComponentEnable ("LteNetDevice", LOG_LEVEL_INFO);
    }

  if (useCa)
    {
      Config::SetDefault ("ns3::LteHelper::UseCa", BooleanValue (useCa));
      Config::SetDefault ("ns3::LteHelper::NumberOfComponentCarriers", UintegerValue (2));
      Config::SetDefault ("ns3::LteHelper::EnbComponentCarrierManager", StringValue ("ns3::RrComponentCarrierManager"));
    }

  Config::SetDefault("ns3::LteEnbNetDevice::UlBandwidth", UintegerValue(50));
  Config::SetDefault("ns3::LteEnbNetDevice::DlBandwidth", UintegerValue(50));

  // Enable checksum computations (required by OFSwitch13 module)
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
  // To Print packets
  // ns3::PacketMetadata::Enable ();

  // Create the LTE infrastructure
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);

  // ********************************************************************
  // ConfigStore inputConfig;
  // inputConfig.ConfigureDefaults();
  // // parse again so you can override default values from the command line
  // cmd.Parse(argc, argv);
  // ********************************************************************

  Ptr<Node> pgw = epcHelper->GetPgwNode ();

  // Create the controller node
  Ptr<Node> controllerNode = CreateObject<Node> ();

  // Create the switch node
  Ptr<Node> switchNode = CreateObject<Node> ();

  NodeContainer ueNodes;
  NodeContainer enbNodes;
  NodeContainer ueHostNode;
  enbNodes.Create(3);
  ueNodes.Add(controllerNode);
  ueNodes.Add(switchNode);
  ueHostNode.Create(1);
  ueNodes.Add(ueHostNode);

  // Install Mobility Model to LTE nodes
  Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
  Ptr<ListPositionAllocator> uePositionAlloc = CreateObject<ListPositionAllocator> ();

  enbPositionAlloc->Add (Vector( 0, 10, 0));
  enbPositionAlloc->Add (Vector(50, 10, 0));
  enbPositionAlloc->Add (Vector(60,  0, 0));
  uePositionAlloc->Add (Vector( 0, 0, 0));
  uePositionAlloc->Add (Vector(50, 0, 0));
  uePositionAlloc->Add (Vector(70, 0, 0));

  MobilityHelper mobility;
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator(enbPositionAlloc);
  mobility.Install(enbNodes);
  mobility.SetPositionAllocator(uePositionAlloc);
  mobility.Install(ueNodes);

  // Install LTE Devices to the nodes
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);
  // Install one more LTE device to the switch node
  ueLteDevs.Add ( (lteHelper->InstallUeDevice ( NodeContainer(switchNode) )).Get (0));

  for (NetDeviceContainer::Iterator it = ueLteDevs.Begin(); it != ueLteDevs.End() ; it++ )
    {
      std::cout << "UE Device address: " << (*it)->GetAddress () << std::endl;
    }

  for (NetDeviceContainer::Iterator it = enbLteDevs.Begin(); it != enbLteDevs.End() ; it++ )
    {
      std::cout << "eNB Device address: " << (*it)->GetAddress () << std::endl;
    }

  // Create three host nodes
  NodeContainer hosts;
  hosts.Create (3);

  // Use the CsmaHelper to connect host nodes to the switch node
  CsmaHelper csmaHelper;
  csmaHelper.SetChannelAttribute ("DataRate", DataRateValue (DataRate ("100Mbps")));
  csmaHelper.SetChannelAttribute ("Delay", TimeValue (MilliSeconds (2)));

  NetDeviceContainer hostDevices;
  NetDeviceContainer switchPorts;
  for (size_t i = 0; i < hosts.GetN (); i++)
    {
      NodeContainer pair (hosts.Get (i), switchNode);
      NetDeviceContainer link = csmaHelper.Install (pair);
      hostDevices.Add (link.Get (0));
      switchPorts.Add (link.Get (1));
    }
  // Fourth switch port is the LTE Net Device
  switchPorts.Add (ueLteDevs.Get (3));

  // Configure the OpenFlow network domain
  Ptr<OFSwitch13InternalHelper> of13Helper = CreateObject<OFSwitch13InternalHelper> ();
  Ptr<TrafficTypeController> vanetCtrl = CreateObject<TrafficTypeController> ();
  of13Helper->InstallController (controllerNode, vanetCtrl);
  of13Helper->InstallSwitch (switchNode, switchPorts);
  // of13Helper->CreateOpenFlowChannels ();
  of13Helper->GetCtlDevs().Add(ueLteDevs.Get(0));
  // Install IPv4 stack to ueHostNode (in first two ueNodes it was installed when controller and Switch were installed)
  InternetStackHelper internet;
  internet.Install (ueHostNode);

  // Assign IP address to LTE UEs
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (ueLteDevs));
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  // Attach one UE per eNodeB
  // UEs need to have IPv4/IPv6 installed before EPS bearers can be activated
  for (uint16_t i = 0; i < enbLteDevs.GetN () ; i++)
      {
        lteHelper->Attach (ueLteDevs.Get(i), enbLteDevs.Get(i));
        // side effect: the default EPS bearer will be activated
      }
  // Finally, the switch node's second LTE device is attached to the last enbLteDev
  lteHelper->Attach (ueLteDevs.Get(3), enbLteDevs.Get(2));

  // Start the connections between controllers and switches.
  UintegerValue portValue;
  of13Helper->GetCtlApps().Get(0)->GetAttribute ("Port", portValue);
  // Controller Application Socket Address
  InetSocketAddress addr (ueIpIface.GetAddress (0), portValue.Get ());
  uint64_t dpId = of13Helper->GetOFDevs().Get(0)->GetDatapathId ();
  NS_LOG_INFO ("Connect switch " <<  dpId <<
               " to controller " << addr.GetIpv4 () << " port " << addr.GetPort ());

  Simulator::ScheduleNow (&OFSwitch13Device::StartControllerConnection, of13Helper->GetOFDevs().Get(0), addr);

  // Install the TCP/IP stack into hosts nodes
  internet.Install (hosts);

  // Set IPv4 host addresses
  Ipv4AddressHelper ipv4helpr;
  Ipv4InterfaceContainer hostIpIfaces;
  // Skipping first ip address reserved for default gateway
  ipv4helpr.SetBase ("10.1.1.0", "255.255.255.0", "0.0.0.2");
  hostIpIfaces = ipv4helpr.Assign (hostDevices);

  for (uint16_t i = 0; i < 3; i++)
    {
      // Set the default gateway for hosts
      Ptr<Ipv4StaticRouting> hostStaticRouting = ipv4RoutingHelper.GetStaticRouting (hosts.Get (i)->GetObject<Ipv4> ());
      hostStaticRouting->SetDefaultRoute (Ipv4Address ("10.1.1.1"), 1);
    }

  // Configure ping application between hosts
  // V4PingHelper pingHelper1 = V4PingHelper (hostIpIfaces.GetAddress (0));
  V4PingHelper pingHelper1 = V4PingHelper (Ipv4Address ("7.0.0.4"));
  pingHelper1.SetAttribute ("Verbose", BooleanValue (true));
  // V4PingHelper pingHelper2 = V4PingHelper (hostIpIfaces.GetAddress (1));
  // pingHelper2.SetAttribute ("Verbose", BooleanValue (true));
  ApplicationContainer pingApps = pingHelper1.Install (hosts.Get (2));
  // pingApps.Add(pingHelper2.Install (hosts.Get (2)));

  pingApps.Start (Seconds (4));

  // Enable datapath stats and pcap traces at hosts, switch(es), and controller(s)
  if (trace)
    {
      // of13Helper->EnableOpenFlowPcap ("openflow");
      // of13Helper->EnableDatapathStats ("switch-stats");
      csmaHelper.EnablePcap ("switch", switchPorts, true);
      csmaHelper.EnablePcap ("host", hostDevices);
    }

  csmaHelper.EnablePcap ("host", hostDevices.Get(1));

  // Run the simulation
  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();
  Simulator::Destroy ();
}
