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

 //802.11p Includes
 #include "ns3/yans-wifi-helper.h"
 #include "ns3/ocb-wifi-mac.h"
 #include "ns3/wifi-80211p-helper.h"
 #include "ns3/wave-mac-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("SecondSDNthroughLTE");

int
main (int argc, char *argv[])
{
  uint16_t simTime = 6;
  bool verbose = false;
  bool trace = false;
  //Lte
  double distance = 60.0;
  bool useCa = false;
  //802.11p
  std::string phyMode ("OfdmRate6MbpsBW10MHz");
  bool verbose11p = false;

  bool v4pingVerbose = true;

  // Configure command line parameters
  CommandLine cmd;
  cmd.AddValue ("simTime", "Simulation time (seconds)", simTime);
  cmd.AddValue ("verbose", "Enable SDN verbose output", verbose);
  cmd.AddValue ("trace", "Enable datapath stats and pcap traces", trace);
  // Lte
  cmd.AddValue("distance", "Distance between eNBs [m]", distance);
  cmd.AddValue("useCa", "Whether to use carrier aggregation.", useCa);
  // 802.11p
  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("verbose11p", "turn on all WifiNetDevice log components", verbose11p);

  cmd.AddValue ("v4ping-verbose", "turn on log component for V4Ping Application", v4pingVerbose);
  // Parse
  cmd.Parse (argc, argv);

  if (verbose)
    {
      OFSwitch13Helper::EnableDatapathLogs ();
      LogComponentEnable ("OFSwitch13Interface", LOG_LEVEL_ALL);
      LogComponentEnable ("OFSwitch13Device", LOG_LEVEL_ALL);
      LogComponentEnable ("OFSwitch13Port", LOG_LEVEL_ALL);
      LogComponentEnable ("OFSwitch13Queue", LOG_LEVEL_ALL);
      LogComponentEnable ("OFSwitch13SocketHandler", LOG_LEVEL_ALL);
      LogComponentEnable ("OFSwitch13Controller", LOG_LEVEL_ALL);
      LogComponentEnable ("OFSwitch13LearningController", LOG_LEVEL_ALL);
      LogComponentEnable ("OFSwitch13Helper", LOG_LEVEL_ALL);
      LogComponentEnable ("OFSwitch13InternalHelper", LOG_LEVEL_ALL);
      LogComponentEnable ("SecondSDNthroughLTE", LOG_LEVEL_ALL);
    }

  if (useCa)
    {
      Config::SetDefault ("ns3::LteHelper::UseCa", BooleanValue (useCa));
      Config::SetDefault ("ns3::LteHelper::NumberOfComponentCarriers", UintegerValue (2));
      Config::SetDefault ("ns3::LteHelper::EnbComponentCarrierManager", StringValue ("ns3::RrComponentCarrierManager"));
    }

  if (v4pingVerbose)
    {
      // LogComponentEnable ("V4Ping", LOG_LEVEL_INFO);
      // LogComponentEnable ("Ipv4RawSocketImpl", LOG_LEVEL_INFO);
      // LogComponentEnable ("Socket", LOG_LEVEL_INFO);
      // LogComponentEnable ("Ipv4L3Protocol", LOG_LEVEL_INFO);
      // LogComponentEnable ("Ipv4Interface", LOG_LEVEL_INFO);
      // LogComponentEnable ("ArpL3Protocol", LOG_LEVEL_INFO);
      // LogComponentEnable ("TrafficControlLayer", LOG_LEVEL_INFO);
      // LogComponentEnable ("QueueDisc", LOG_LEVEL_INFO);
      // LogComponentEnable ("CsmaNetDevice", LOG_LEVEL_INFO);
      // LogComponentEnable ("CsmaChannel", LOG_LEVEL_INFO);
      // LogComponentEnable ("WifiNetDevice", LOG_LEVEL_INFO);
      // LogComponentEnable ("OcbWifiMac", LOG_LEVEL_INFO);
      // LogComponentEnable ("MacLow", LOG_LEVEL_INFO);
      // // LogComponentEnable ("MacRxMiddle", LOG_LEVEL_INFO);
      // LogComponentEnable ("YansWifiPhy", LOG_LEVEL_INFO);
      // LogComponentEnable ("WifiPhy", LOG_LEVEL_INFO);
      // LogComponentEnable ("YansWifiChannel", LOG_LEVEL_INFO);
      LogComponentEnable ("LteUeNetDevice", LOG_LEVEL_INFO);
    }

    Config::SetDefault("ns3::LteEnbNetDevice::UlBandwidth", UintegerValue(50));
    Config::SetDefault("ns3::LteEnbNetDevice::DlBandwidth", UintegerValue(50));

  // Enable checksum computations (required by OFSwitch13 module)
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
  // To Debug the program
  ns3::PacketMetadata::Enable ();

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

  // Create the OpenFlow controller node
  Ptr<Node> controllerNode = CreateObject<Node> ();

  // Create the OpenFlow switch node
  Ptr<Node> switchNode = CreateObject<Node> ();

  NodeContainer ueNodes;
  NodeContainer enbNodes;
  enbNodes.Create(2);
  ueNodes.Add(controllerNode);
  ueNodes.Add(switchNode);

  // Install Mobility Model to LTE nodes
  Ptr<ListPositionAllocator> positionAlloc = CreateObject<ListPositionAllocator> ();
  for (uint16_t i = 0; i < 2; i++)
    {
      positionAlloc->Add (Vector(distance * i, 0, 0));
    }
  MobilityHelper mobility;
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator(positionAlloc);
  mobility.Install(enbNodes);
  mobility.Install(ueNodes);

  // Install LTE Devices to the nodes
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);

  /**** Create a 802.11p link ****/

  NodeContainer wifi80211pNodes;
  wifi80211pNodes.Create (1);                // We create the 802.11p host node.
  wifi80211pNodes.Add(switchNode);           // The second node is also the OpenFlow Switch

  // The below set of helpers will help us to put together the wifi NICs we want
  YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  Ptr<YansWifiChannel> channel = wifiChannel.Create ();
  wifiPhy.SetChannel (channel);

  NqosWaveMacHelper wifi80211pMac = NqosWaveMacHelper::Default ();
  Wifi80211pHelper wifi80211p = Wifi80211pHelper::Default ();

  if (verbose11p)
    {
      wifi80211p.EnableLogComponents ();  // Turn on all Wifi 802.11p logging
    }

  wifi80211p.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                      "DataMode",StringValue (phyMode),
                                      "ControlMode",StringValue (phyMode));

  // Installl the mobility model in the 802.11p host whose location is related to the LTE Open Flow Switch
  MobilityHelper mobility80211pHost;
  Ptr<ListPositionAllocator> positionAlloc11pNode = CreateObject<ListPositionAllocator> ();
  positionAlloc11pNode->Add (Vector (distance * 1, 5.0, 0.0));
  mobility.SetPositionAllocator (positionAlloc11pNode);
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (wifi80211pNodes.Get (0));
  // Create the Wifi Network Devices
  NetDeviceContainer wifi80211pDevs = wifi80211p.Install (wifiPhy, wifi80211pMac, wifi80211pNodes);

  // Create two ipv4add nodes
  NodeContainer hosts;
  hosts.Create (2);

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

  // We also add the 802.11p interfaces to these device containers
  hostDevices.Add (wifi80211pDevs.Get (0));
  switchPorts.Add (wifi80211pDevs.Get (1));

  // Configure the OpenFlow network domain
  Ptr<OFSwitch13InternalHelper> of13Helper = CreateObject<OFSwitch13InternalHelper> ();
  of13Helper->InstallController (controllerNode);
  of13Helper->InstallSwitch (switchNode, switchPorts);
  // ^ When the switchPorts are added, if the port is a CsmaNetDevice a call to SetQueue () is made on that device
  // ^ Something equivalent need to be done for WifiNetDevice?

  // of13Helper->CreateOpenFlowChannels ();
  // We configure the OpenFlow channel through LTE instead of the default csma or point-to-point
  of13Helper->GetCtlDevs().Add(ueLteDevs.Get(0));

  // Assign IP address to UEs
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
  for (uint16_t i = 0; i < 2; i++)
      {
        lteHelper->Attach (ueLteDevs.Get(i), enbLteDevs.Get(i));
        // side effect: the default EPS bearer will be activated
      }

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
  InternetStackHelper internet;
  // Add the 802.11p host to the hosts container
  hosts.Add (wifi80211pNodes.Get (0));
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
  V4PingHelper pingHelper = V4PingHelper (hostIpIfaces.GetAddress (1));
  // V4PingHelper pingHelper = V4PingHelper (Ipv4Address ("20.1.1.5"));
  pingHelper.SetAttribute ("Verbose", BooleanValue (true));
  ApplicationContainer pingApps = pingHelper.Install (hosts.Get (0));
  pingApps.Start (Seconds (4));

  // Enable datapath stats and pcap traces at hosts, switch(es), and controller(s)
  if (trace)
    {
      // of13Helper->EnableOpenFlowPcap ("openflow");
      // of13Helper->EnableDatapathStats ("switch-stats");
      // csmaHelper.EnablePcap ("switch", switchPorts, true);
      // csmaHelper.EnablePcap ("host", hostDevices);
    }

  // Run the simulation
  Simulator::Stop (Seconds (simTime));
  Simulator::Run ();
  Simulator::Destroy ();
}
