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

using namespace std;
using namespace ns3;
using namespace mmwave;

/**
 * This test version creates a new node for the mmWave interface
 */

/**
 * Sample simulation script for a connected car equipped with an OpenFlow switch
 * which is in turn equipped with interfaces: eth, 802.11p, lte, cellular mmave.
 * Different data sources from the car space are routed through specific interfaces
 * selected according to the logic configured locally or by the SDN controller.
 */

/* This implements sending udp data to/from a local host and a remote server */

/**  Application Variables **/
uint64_t totalRxHost = 0;
double throughputRxHost = 0;
Ptr<PacketSink> packetSinkHost;

uint64_t totalRxServer = 0;
double throughputRxServer = 0;
Ptr<PacketSink> packetSinkServer;

double
CalculateSingleStreamThroughput (Ptr<PacketSink> sink, uint64_t &lastTotalRx, double &averageThroughput)
{
  double thr = (sink->GetTotalRx() - lastTotalRx) * (double) 8/1e5;     /* Convert Application RX Packets to MBits. */
  lastTotalRx = sink->GetTotalRx ();
  averageThroughput += thr;
  return thr;
}

void
CalculateThroughput (void)
{
  double thr = CalculateSingleStreamThroughput (packetSinkHost, totalRxHost, throughputRxHost);
  std::cout << "DL: time " << Simulator::Now ().GetSeconds () << " s\tdata rate " << thr << " Mbps" << std::endl;
  thr = CalculateSingleStreamThroughput (packetSinkServer, totalRxServer, throughputRxServer);
  std::cout << "UL: time " << Simulator::Now ().GetSeconds () << " s\tdata rate " << thr << " Mbps" << std::endl;
  Simulator::Schedule (MilliSeconds (100), &CalculateThroughput);
}

NS_LOG_COMPONENT_DEFINE ("SimpleSdnCarSwitchMmWave");

int
main (int argc, char *argv[])
{
  uint16_t simTime = 5;
  bool verbose = true;  // OpenFlow verbose
  bool trace = false;
  // Lte
  // bool useCa = false;
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
  double interPacketInterval = 100;
  uint32_t payloadSize = 1472;                  /* Application payload size in bytes. */
  string dataRate = "100Mbps";                  /* Application data rate. */

  NS_UNUSED (interPacketInterval);
  NS_UNUSED (payloadSize);

  // Configure command line parameters
  CommandLine cmd;
  cmd.AddValue ("simTime", "Simulation time (seconds)", simTime);
  cmd.AddValue ("verbose", "Enable verbose output", verbose);
  cmd.AddValue ("trace", "Enable datapath stats and pcap traces", trace);
  // Lte
  // cmd.AddValue("useCa", "Whether to use carrier aggregation.", useCa);
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
  // if (useCa)
  //   {
  //     Config::SetDefault ("ns3::LteHelper::UseCa", BooleanValue (useCa));
  //     Config::SetDefault ("ns3::LteHelper::NumberOfComponentCarriers", UintegerValue (2));
  //     Config::SetDefault ("ns3::LteHelper::EnbComponentCarrierManager", StringValue ("ns3::RrComponentCarrierManager"));
  //   }
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
      // LogComponentEnable ("OFSwitch13Device", LOG_LEVEL_ALL);
      // LogComponentEnable ("OFSwitch13Port", LOG_LEVEL_ALL);
      //LogComponentEnable ("OFSwitch13Queue", LOG_LEVEL_ALL);
      //LogComponentEnable ("OFSwitch13SocketHandler", LOG_LEVEL_ALL);
      LogComponentEnable ("OFSwitch13Controller", LOG_LEVEL_ALL);
      //LogComponentEnable ("OFSwitch13LearningController", LOG_LEVEL_ALL);
      //LogComponentEnable ("OFSwitch13Helper", LOG_LEVEL_ALL);
      //LogComponentEnable ("OFSwitch13InternalHelper", LOG_LEVEL_ALL);
    }
    LogComponentEnable ("SimpleSdnCarSwitchMmWave", LOG_LEVEL_INFO);
    // LogComponentEnable ("TypeId", LOG_LEVEL_ALL);
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
      // LogComponentEnable ("CsmaNetDevice", LOG_LEVEL_INFO);
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

  // Create the Lte + cellular mm Wave infrastructure
  Ptr<MmWaveHelper> mmWaveHelper = CreateObject<MmWaveHelper> ();
  mmWaveHelper->SetSchedulerType ("ns3::MmWaveFlexTtiMacScheduler");
  Ptr<MmWavePointToPointEpcHelper>  epcHelper = CreateObject<MmWavePointToPointEpcHelper> ();
  mmWaveHelper->SetEpcHelper (epcHelper);
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
  NodeContainer lteEnbNodes;
  NodeContainer mmWaveEnbNodes;
  ueNodes.Add(controllerNode);
  ueNodes.Add(switchNode);
  lteEnbNodes.Create(1);
  mmWaveEnbNodes.Create(1);

  NodeContainer mmWaveNodeContainer;
  mmWaveNodeContainer.Create (1);

  // Reserve Mac Address 00:00:00:00:00:01
  Mac48Address::Allocate();

  // Create an Internet Server
  NodeContainer remoteServerContainer;
  remoteServerContainer.Create(1);
  Ptr<Node> remoteServer = remoteServerContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteServerContainer);

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteServer);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  // interface 0 is localhost, 1 is the p2p remote server
  Ipv4Address remoteServerAddr = internetIpIfaces.GetAddress (1);
  NS_LOG_INFO ("Remote Server Address:\t" << remoteServerAddr);
  // Add to remote server a route to the local host
  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteServerStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteServer->GetObject<Ipv4> ());
  remoteServerStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  // Install Mobility Model to LTE and mmWave nodes
  MobilityHelper lteMobility;
  Ptr<ListPositionAllocator> ltePositionAlloc = CreateObject<ListPositionAllocator> ();
  // lte enb
  ltePositionAlloc->Add (Vector(100, 400, 20));
  // mm Wave enb
  ltePositionAlloc->Add (Vector(196, 128, 20));
  // Ue nodes
  // ltePositionAlloc->Add (Vector( 0, 500, 1)); //SDN Controller
  ltePositionAlloc->Add (Vector(100,156, 1)); //SDN Controller
  ltePositionAlloc->Add (Vector(100,100, 1)); //Car with OpenFlow Switch
  // mmWaveNode
  ltePositionAlloc->Add (Vector(100,100, 1)); //Same location of the Car Switch

  lteMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  lteMobility.SetPositionAllocator(ltePositionAlloc);
  lteMobility.Install(lteEnbNodes);
  lteMobility.Install(mmWaveEnbNodes);
  lteMobility.Install(ueNodes);
  lteMobility.Install(mmWaveNodeContainer);

  // std::cout << "Controller location: " << ueNodes.Get(0)->GetObject<MobilityModel> ()->GetPosition() << std::endl;
  // std::cout << "Switch location: " << ueNodes.Get(1)->GetObject<MobilityModel> ()->GetPosition() << std::endl;
  // std::cout << "Lte Tower location: " << lteEnbNodes.Get(0)->GetObject<MobilityModel> ()->GetPosition() << std::endl;
  // std::cout << "Cellular mmWave Tower location: " << mmWaveEnbNodes.Get(0)->GetObject<MobilityModel> ()->GetPosition() << std::endl;

  NS_LOG_INFO ("Controller location:\t" << ueNodes.Get(0)->GetObject<MobilityModel> ()->GetPosition());
  NS_LOG_INFO ("Switch location:\t" << ueNodes.Get(1)->GetObject<MobilityModel> ()->GetPosition());
  NS_LOG_INFO ("Lte Tower location:\t" << lteEnbNodes.Get(0)->GetObject<MobilityModel> ()->GetPosition());
  NS_LOG_INFO ("Cellular mmWave Tower location: " << mmWaveEnbNodes.Get(0)->GetObject<MobilityModel> ()->GetPosition());

  // Install LTE and mmWave Devices to the nodes
  NetDeviceContainer lteEnbDevs = mmWaveHelper->InstallLteEnbDevice (lteEnbNodes);
  NetDeviceContainer mmWaveEnbDevs = mmWaveHelper->InstallEnbDevice (mmWaveEnbNodes);
  // For an unknown reason, only by installing first the mmWave Ue device to the switch node, the simulation does ot throw error
  // NetDeviceContainer mmWaveUeDevs = mmWaveHelper->InstallUeDevice ( NodeContainer(switchNode) );
  // NetDeviceContainer lteUeDevs = mmWaveHelper->InstallLteUeDevice (ueNodes);
  // NetDeviceContainer lteUeDevs = mmWaveHelper->InstallMcUeDevice (ueNodes);
  NetDeviceContainer mmWaveUeDevs = mmWaveHelper->InstallUeDevice (ueNodes);
  NetDeviceContainer lteUeDevs = mmWaveHelper->InstallLteUeDevice (NodeContainer(switchNode));

  // Install one more LTE device to the switch node
  // lteUeDevs.Add (mmWaveHelper->InstallLteUeDevice ( NodeContainer(switchNode) ));
  // lteUeDevs.Add (mmWaveHelper->InstallMcUeDevice ( NodeContainer(switchNode) ));

  // NetDeviceContainer mmWaveUeDevs = mmWaveHelper->InstallUeDevice ( mmWaveNodeContainer );

  for (NetDeviceContainer::Iterator it = lteUeDevs.Begin(); it != lteUeDevs.End() ; it++ )
    {
      // std::cout << "Lte UE Device address: " << (*it)->GetAddress () << std::endl;
      NS_LOG_INFO ("Lte UE Device address:\t" << (*it)->GetAddress ());
    }

  for (NetDeviceContainer::Iterator it = mmWaveUeDevs.Begin(); it != mmWaveUeDevs.End() ; it++ )
    {
      // std::cout << "mmWave UE Device address: " << (*it)->GetAddress () << std::endl;
      NS_LOG_INFO ("mmWave UE Device address: " << (*it)->GetAddress ());
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
      NS_LOG_INFO ("New Host with CSMA Net Device address: " << link.Get(0)->GetAddress());
    }
  // Fourth switch port is the LTE Net Device
  // switchPorts.Add (lteUeDevs.Get (2));
  switchPorts.Add (lteUeDevs.Get (0));
  // Fifth switch port is the mm Wave Net Device
  // switchPorts.Add (mmWaveUeDevs.Get (0));

  // Configure the OpenFlow network domain
  Ptr<OFSwitch13InternalHelper> of13Helper = CreateObject<OFSwitch13InternalHelper> ();
  Ptr<TrafficTypeController> vanetCtrl = CreateObject<TrafficTypeController> ();
  of13Helper->InstallController (controllerNode, vanetCtrl);
  of13Helper->InstallSwitch (switchNode, switchPorts);
  /* of13Helper->CreateOpenFlowChannels (); */
  // of13Helper->GetCtlDevs().Add(lteUeDevs.Get(0));
  of13Helper->GetCtlDevs().Add(mmWaveUeDevs.Get(0));

  // internet.Install (NodeContainer ( NodeContainer(controllerNode), NodeContainer(switchNode), mmWaveNodeContainer));
  internet.Install (NodeContainer (mmWaveNodeContainer));
  // Assign IP address to LTE UEs and attach them to eNBs
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (lteUeDevs, mmWaveUeDevs));
  // ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (mmWaveUeDevs));
  for (size_t i = 0; i < ueIpIface.GetN (); i++ )
    {
      // std::cout << "Lte UE Device address: " << (*it)->GetAddress () << std::endl;
      NS_LOG_INFO ("UE Device IP address:\t" << ueIpIface.GetAddress (i));
    }
  //TODO check to see if this is needed in this test case where lte devs are installed in te Open Flow Switch:
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

  // Attach UEs to eNB nodes
  /* UEs need to have IPv4/IPv6 installed before EPS bearers can be activated
     side effect: the default EPS bearer will be activated */
  mmWaveHelper->AttachToClosestLteEnb (lteUeDevs, lteEnbDevs);
  mmWaveHelper->AttachToClosestEnb (mmWaveUeDevs, mmWaveEnbDevs);

  // Start the connections between SDN controller and switches.
  UintegerValue portValue;
  of13Helper->GetCtlApps().Get(0)->GetAttribute ("Port", portValue);
  // Controller Application Socket Address
  InetSocketAddress addr (ueIpIface.GetAddress (1), portValue.Get ());
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
  // V4PingHelper pingHelper1 = V4PingHelper (remoteServerAddr);
  // pingHelper1.SetAttribute ("Verbose", BooleanValue (true));
  // V4PingHelper pingHelper2 = V4PingHelper (hostIpIfaces.GetAddress (1));
  // pingHelper2.SetAttribute ("Verbose", BooleanValue (true));

  // ApplicationContainer pingApps = pingHelper1.Install (hosts.Get (2));

  // pingApps.Add(pingHelper2.Install (hosts.Get (2)));

  // pingApps.Start (Seconds (4));

  /* UDP Data */
  uint16_t dlPort = 1000;
  uint16_t ulPort = 2000;

  ApplicationContainer sinkApps;
  ApplicationContainer srcApps;

  /* Sink Apps */
  PacketSinkHelper ulsinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ulPort));
  sinkApps.Add (ulsinkHelper.Install (remoteServer));
  PacketSinkHelper dlsinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
  sinkApps.Add (dlsinkHelper.Install (hosts.Get(2)));
  packetSinkServer = StaticCast<PacketSink> (sinkApps.Get (0));
  packetSinkHost = StaticCast<PacketSink> (sinkApps.Get (1));
  /* UDP Transmitter */
  OnOffHelper srcHost ("ns3::UdpSocketFactory", InetSocketAddress (remoteServerAddr, ulPort));
  srcHost.SetAttribute ("MaxBytes", UintegerValue (0));
  srcHost.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  srcHost.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1e6]"));
  srcHost.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  srcHost.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
  srcApps.Add (srcHost.Install(hosts.Get(2)));
  OnOffHelper srcServer ("ns3::UdpSocketFactory", InetSocketAddress ("7.0.0.2", dlPort));
  srcServer.SetAttribute ("MaxBytes", UintegerValue (0));
  srcServer.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  srcServer.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1e6]"));
  srcServer.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  srcServer.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
  srcApps.Add (srcServer.Install(remoteServer));

  sinkApps.Start (Seconds (1.0));
  srcApps.Start (Seconds (3.9));

  Simulator::Schedule (Seconds (4), &CalculateThroughput);

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
