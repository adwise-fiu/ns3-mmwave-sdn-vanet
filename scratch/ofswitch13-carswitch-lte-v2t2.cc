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
 #include "ns3/ipv4-global-routing-helper.h"
 #include "ns3/mobility-module.h"
 #include "ns3/lte-module.h"
 #include "ns3/applications-module.h"
 #include "ns3/point-to-point-helper.h"
 #include "ns3/config-store.h"

 //5G LTE includes
 #include "ns3/mmwave-helper.h"
 #include "ns3/mmwave-point-to-point-epc-helper.h"

using namespace ns3;
using namespace mmwave;

NS_LOG_COMPONENT_DEFINE ("SimpleSDNCarSwitcht5GLTE");

int
main (int argc, char *argv[])
{
  uint16_t simTime = 6;
  bool verbose = true;  // OpenFlow verbose
  bool trace = false;
  // Lte
  double distance = 60.0;
  bool useCa = false;
  // mm Wave Lte
  bool harqEnabled = true;
	bool rlcAmEnabled = false;
	bool fixedTti = false;
	unsigned symPerSf = 24;
	double sfPeriod = 100.0;
	unsigned run = 0;
	bool smallScale = true;
	double speed = 3;    // For doppler effect
  // Application
  bool v4pingVerbose = true;

  // Configure command line parameters
  CommandLine cmd;
  cmd.AddValue ("simTime", "Simulation time (seconds)", simTime);
  cmd.AddValue ("verbose", "Enable verbose output", verbose);
  cmd.AddValue ("trace", "Enable datapath stats and pcap traces", trace);
  // Lte
  cmd.AddValue("distance", "Distance between eNBs [m]", distance);
  cmd.AddValue("useCa", "Whether to use carrier aggregation.", useCa);
  // mm Wave Lte
  cmd.AddValue("harq", "Enable Hybrid ARQ", harqEnabled);
	cmd.AddValue("rlcAm", "Enable RLC-AM", rlcAmEnabled);
	cmd.AddValue("symPerSf", "OFDM symbols per subframe", symPerSf);
	cmd.AddValue("sfPeriod", "Subframe period = 4.16 * symPerSf", sfPeriod);
	cmd.AddValue("fixedTti", "Fixed TTI scheduler", fixedTti);
	cmd.AddValue("run", "run for RNG (for generating different deterministic sequences for different drops)", run);
  // Application
  cmd.AddValue ("v4ping-verbose", "turn on log component for V4Ping Application", v4pingVerbose);

  cmd.Parse (argc, argv);

  // Lte
  if (useCa)
    {
      Config::SetDefault ("ns3::LteHelper::UseCa", BooleanValue (useCa));
      Config::SetDefault ("ns3::LteHelper::NumberOfComponentCarriers", UintegerValue (2));
      Config::SetDefault ("ns3::LteHelper::EnbComponentCarrierManager", StringValue ("ns3::RrComponentCarrierManager"));
    }
  Config::SetDefault("ns3::LteEnbNetDevice::UlBandwidth", UintegerValue(50));
  Config::SetDefault("ns3::LteEnbNetDevice::DlBandwidth", UintegerValue(50));

  // mm Wave Lte
  Config::SetDefault ("ns3::MmWaveHelper::RlcAmEnabled", BooleanValue(rlcAmEnabled));
  Config::SetDefault ("ns3::MmWaveHelper::HarqEnabled", BooleanValue(harqEnabled));
  Config::SetDefault ("ns3::MmWaveFlexTtiMacScheduler::HarqEnabled", BooleanValue(harqEnabled));
  Config::SetDefault ("ns3::MmWaveFlexTtiMacScheduler::CqiTimerThreshold", UintegerValue(1000));
  Config::SetDefault ("ns3::MmWaveFlexTtiMaxWeightMacScheduler::HarqEnabled", BooleanValue(harqEnabled));
  Config::SetDefault ("ns3::MmWaveFlexTtiMaxWeightMacScheduler::FixedTti", BooleanValue(fixedTti));
  Config::SetDefault ("ns3::MmWaveFlexTtiMaxWeightMacScheduler::SymPerSlot", UintegerValue(6));
  Config::SetDefault ("ns3::MmWavePhyMacCommon::ResourceBlockNum", UintegerValue(1));
  Config::SetDefault ("ns3::MmWavePhyMacCommon::ChunkPerRB", UintegerValue(72));
  Config::SetDefault ("ns3::MmWavePhyMacCommon::SymbolsPerSubframe", UintegerValue(symPerSf));
  Config::SetDefault ("ns3::MmWavePhyMacCommon::SubframePeriod", DoubleValue(sfPeriod));
  Config::SetDefault ("ns3::MmWavePhyMacCommon::TbDecodeLatency", UintegerValue(200.0));
  Config::SetDefault ("ns3::MmWaveBeamforming::LongTermUpdatePeriod", TimeValue (MilliSeconds (100.0)));
  Config::SetDefault ("ns3::LteEnbRrc::SystemInformationPeriodicity", TimeValue (MilliSeconds (5.0)));
  // Config::SetDefault ("ns3::MmWavePropagationLossModel::ChannelStates", StringValue ("n"));
  Config::SetDefault ("ns3::LteRlcAm::ReportBufferStatusTimer", TimeValue(MicroSeconds(100.0)));
  Config::SetDefault ("ns3::LteRlcUmLowLat::ReportBufferStatusTimer", TimeValue(MicroSeconds(100.0)));
  Config::SetDefault ("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue (320));
  Config::SetDefault ("ns3::LteEnbRrc::FirstSibTime", UintegerValue (2));
  Config::SetDefault ("ns3::MmWaveBeamforming::SmallScaleFading", BooleanValue (smallScale));
  Config::SetDefault ("ns3::MmWaveBeamforming::FixSpeed", BooleanValue (true));
  Config::SetDefault ("ns3::MmWaveBeamforming::UeSpeed", DoubleValue (speed));

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

  // Enable checksum computations (required by OFSwitch13 module)
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
  // To enable print packets
  // ns3::PacketMetadata::Enable ();

  // *******************************
  // RngSeedManager::SetSeed (1234);
	// RngSeedManager::SetRun (run);
  // *******************************

  // Create the LTE infrastructure
  Ptr<LteHelper> lteHelper = CreateObject<LteHelper> ();
  Ptr<PointToPointEpcHelper>  epcHelper = CreateObject<PointToPointEpcHelper> ();
  lteHelper->SetEpcHelper (epcHelper);
  // mm Wave Lte
  Ptr<MmWaveHelper> mmWaveHelper = CreateObject<MmWaveHelper> ();
  mmWaveHelper->SetSchedulerType ("ns3::MmWaveFlexTtiMacScheduler");
  Ptr<MmWavePointToPointEpcHelper>  epcHelper5g = CreateObject<MmWavePointToPointEpcHelper> ();
  mmWaveHelper->SetEpcHelper (epcHelper5g);
  mmWaveHelper->SetHarqEnabled (harqEnabled);

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
  enbNodes.Create(1);
  ueNodes.Add(controllerNode);
  ueNodes.Add(switchNode);

  Ptr<Node> pgw5g = epcHelper5g->GetPgwNode ();
  NodeContainer ueNodes5g;
  NodeContainer enbNodes5g;
  enbNodes5g.Create(1);
  // ueNodes5g.Add(switchNode);

  // Create the remote host node that will be equipped with Lte and 5G Lte interfaces
  NodeContainer ueServerNodeC;
  ueServerNodeC.Create(1);

  // NodeContainer ueServer2NodeC;
  ueNodes5g.Create(2);


  // Install Mobility Model to LTE nodes
  // Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
  // Ptr<ListPositionAllocator> uePositionAlloc = CreateObject<ListPositionAllocator> ();
  Ptr<ListPositionAllocator> ltePositionAlloc = CreateObject<ListPositionAllocator> ();
  // enb
  ltePositionAlloc->Add (Vector(  0, 5000, 0));
  // ltePositionAlloc->Add (Vector(200, 50, 0));
  // ltePositionAlloc->Add (Vector(296, 28, 0));
  // mm Wave enb
  // ltePositionAlloc->Add (Vector(296,  0, 0));
  ltePositionAlloc->Add (Vector(246,  0, 0));
  // Ue nodes
  ltePositionAlloc->Add (Vector( 0, 100, 0));
  ltePositionAlloc->Add (Vector(200,  0, 0));
  // ltePositionAlloc->Add (Vector(392,  0, 0));
  ltePositionAlloc->Add (Vector(292,  0, 0));
  ltePositionAlloc->Add (Vector(200,  0, 0));
  ltePositionAlloc->Add (Vector(292,  0, 0));

  MobilityHelper mobility;
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator(ltePositionAlloc);
  mobility.Install(enbNodes);
  mobility.Install(enbNodes5g);
  mobility.Install(ueNodes);
  mobility.Install(ueServerNodeC);
  mobility.Install(ueNodes5g);

  std::cout << "Controller location: " << ueNodes.Get(0)->GetObject<MobilityModel> ()->GetPosition() << std::endl;
  std::cout << "Switch location: " << ueNodes.Get(1)->GetObject<MobilityModel> ()->GetPosition() << std::endl;
  std::cout << "Server location: " << ueServerNodeC.Get(0)->GetObject<MobilityModel> ()->GetPosition() << std::endl;
  std::cout << "5G Lte Tower location: " << enbNodes5g.Get(0)->GetObject<MobilityModel> ()->GetPosition() << std::endl;

  // Install LTE and mmWave Devices to the nodes

  NetDeviceContainer enbmmWaveDevs = mmWaveHelper->InstallEnbDevice (enbNodes5g);
  NetDeviceContainer enbLteDevs = lteHelper->InstallEnbDevice (enbNodes);

  // NetDeviceContainer ueLteDevs = lteHelper->InstallUeDevice (ueNodes);
  // Install one more LTE device to the switch node and one 5G LTE device
  // ueLteDevs.Add ( (lteHelper->InstallUeDevice ( NodeContainer(switchNode) )).Get (0));
  // NetDeviceContainer uemmWaveDevs = mmWaveHelper->InstallUeDevice ( NodeContainer(switchNode) );
  NetDeviceContainer uemmWaveDevs = mmWaveHelper->InstallUeDevice (ueNodes5g);
  // Install Lte mm Wave devices to the Server Node
  // ueLteDevs.Add ( (lteHelper->InstallUeDevice (ueServerNodeC)).Get (0));
  // uemmWaveDevs.Add ( (mmWaveHelper->InstallUeDevice (ueServer2NodeC)).Get (0));

  // NetDeviceContainer tempContainerUe = lteHelper->InstallUeDevice (ueServerNodeC);
  // std::cout << "Lte Ue devices to be added: " << tempContainerUe.GetN() << std::endl;
  // ueLteDevs.Add ( tempContainerUe.Get (0));

  // NetDeviceContainer tempContainerMmWaveUe = mmWaveHelper->InstallUeDevice (ueServer2NodeC);
  // std::cout << "mmWave Lte Ue devices to be added: " << tempContainerMmWaveUe.GetN() << std::endl;
  // uemmWaveDevs.Add ( tempContainerMmWaveUe.Get (0));

  // for (NetDeviceContainer::Iterator it = ueLteDevs.Begin(); it != ueLteDevs.End() ; it++ )
  //   {
  //     std::cout << "UE Lte Device address: " << (*it)->GetAddress () << std::endl;
  //   }

  for (NetDeviceContainer::Iterator it = uemmWaveDevs.Begin(); it != uemmWaveDevs.End() ; it++ )
    {
      std::cout << "UE mm Wave Lte Device address: " << (*it)->GetAddress () << std::endl;
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
  // switchPorts.Add (ueLteDevs.Get (2));
  // Fifth switch port is the mm Wave LTE Net Device
  // switchPorts.Add (uemmWaveDevs.Get (0));

  // Configure the OpenFlow network domain
  Ptr<OFSwitch13InternalHelper> of13Helper = CreateObject<OFSwitch13InternalHelper> ();
  Ptr<TrafficTypeController> vanetCtrl = CreateObject<TrafficTypeController> ();
  of13Helper->InstallController (controllerNode, vanetCtrl);
  of13Helper->InstallSwitch (switchNode, switchPorts);
  // of13Helper->CreateOpenFlowChannels ();
  // of13Helper->GetCtlDevs().Add(ueLteDevs.Get(0));
  // Install IPv4 stack to ueHostNode (in first two ueNodes it was installed when controller and Switch were installed)
  InternetStackHelper internet;
  internet.Install (ueServerNodeC);
  // internet.Install (ueServer2NodeC);
  internet.Install (ueNodes5g);

  // ueNodes.Add(ueServerNode);
  // ueNodes5g.Add(ueServer2NodeC);

  // Assign IP address to LTE UEs and attach them to eNBs
  // Ipv4InterfaceContainer ueIpIface;
  // ueIpIface = epcHelper->AssignUeIpv4Address (ueLteDevs);
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  // for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
  //   {
  //     Ptr<Node> ueNode = ueNodes.Get (u);
  //     // Set the default gateway for the UE
  //     Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
  //     ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
  //   }
  // Attach UEs to eNodeBs
  // UEs need to have IPv4/IPv6 installed before EPS bearers can be activated

  // // Controller node
  // lteHelper->Attach (ueLteDevs.Get(0), enbLteDevs.Get(0));
  // // Switch node's openflow channel lte device
  // lteHelper->Attach (ueLteDevs.Get(1), enbLteDevs.Get(1));
  // // Switch node's second LTE device
  // lteHelper->Attach (ueLteDevs.Get(2), enbLteDevs.Get(2));
  // // Remote Server
  // lteHelper->Attach (ueLteDevs.Get(3), enbLteDevs.Get(2));
  // // side effect: the default EPS bearer will be activated

  // Assign IP address to mm Wave UEs and attach them to eNBs
  Ipv4InterfaceContainer ueIpIface5g;
  ueIpIface5g = epcHelper5g->AssignUeIpv4Address (NetDeviceContainer (uemmWaveDevs));
  for (uint32_t u = 0; u < ueNodes5g.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes5g.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper5g->GetUeDefaultGatewayAddress (), 1);
    }

  mmWaveHelper->AttachToClosestEnb (uemmWaveDevs, enbmmWaveDevs);

  // Start the connections between controllers and switches.
  UintegerValue portValue;
  of13Helper->GetCtlApps().Get(0)->GetAttribute ("Port", portValue);
  // Controller Application Socket Address
  // InetSocketAddress addr (ueIpIface.GetAddress (0), portValue.Get ());
  // uint64_t dpId = of13Helper->GetOFDevs().Get(0)->GetDatapathId ();
  // NS_LOG_INFO ("Connect switch " <<  dpId <<
  //              " to controller " << addr.GetIpv4 () << " port " << addr.GetPort ());

  // Simulator::ScheduleNow (&OFSwitch13Device::StartControllerConnection, of13Helper->GetOFDevs().Get(0), addr);

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
  // V4PingHelper pingHelper1 = V4PingHelper (Ipv4Address ("7.0.0.7"));
  V4PingHelper pingHelper1 = V4PingHelper (ueIpIface5g.GetAddress (1));
  pingHelper1.SetAttribute ("Verbose", BooleanValue (true));
  // V4PingHelper pingHelper2 = V4PingHelper (hostIpIfaces.GetAddress (1));
  // pingHelper2.SetAttribute ("Verbose", BooleanValue (true));
  //ApplicationContainer pingApps = pingHelper1.Install (hosts.Get (2));
  // pingApps.Add(pingHelper2.Install (hosts.Get (2)));

  //pingApps.Start (Seconds (4));

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
