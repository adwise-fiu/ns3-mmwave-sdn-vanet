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

//802.11p Includes
#include "ns3/yans-wifi-helper.h"
#include "ns3/ocb-wifi-mac.h"
#include "ns3/wifi-80211p-helper.h"
#include "ns3/wave-mac-helper.h"

// #include "common-functions.h"

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

/**  Application Variables **/
uint64_t totalRxHost = 0;
double throughputRxHost = 0;
Ptr<StreamingApplication> packetSinkHost;

uint64_t totalRxServer = 0;
double throughputRxServer = 0;
Ptr<StreamingApplication> packetSinkServer;

std::string g_courseChangeFile = "vanetMobility.csv";

double
CalculateSingleStreamThroughput (Ptr<StreamingApplication> sink, uint64_t &lastTotalRx, double &averageThroughput)
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
  std::cout << "DL: time " << Simulator::Now ().GetSeconds () << "s\tdata rate " << thr << " Mbps" << std::endl;
  // thr = CalculateSingleStreamThroughput (packetSinkServer, totalRxServer, throughputRxServer);
  // std::cout << "UL: time " << Simulator::Now ().GetSeconds () << "s\tdata rate " << thr << " Mbps" << std::endl;
  Simulator::Schedule (MilliSeconds (100), &CalculateThroughput);
}

void
CourseChange (std::string context, Ptr<const MobilityModel> model)
{
  std::ofstream osf (g_courseChangeFile.c_str(), std::ios::out | std::ios::app);
  if (!osf.is_open())
  {
    std::cerr << "Error: Can't open File " << g_courseChangeFile << "\n";
    return;
  }
  uint8_t index = 10;
  while (context[index] != '/')
  {
    osf << context[index];
    index++;
  }
  osf << "," << Simulator::Now();
  osf << "," << model->GetPosition() << "," << model->GetVelocity() << std::endl;
  osf.close();
}

void
ExportMobility (std::string stage)
{
  std::ofstream osf;
  if (stage == "start") {
    osf.open (g_courseChangeFile.c_str());
    osf << "Node,Time,Position,Velocity" << std::endl;
  }
  else {
    osf.open (g_courseChangeFile.c_str(), std::ios::app);
  }
  // int counter = 0;
  for (NodeContainer::Iterator j = NodeList::Begin() +2; j != NodeList::End() -1; ++j) {
      // if (counter < 2)
      // {
      //   std::cout << "passed node " << counter << std::endl;
      //   counter++;
      //   continue;
      // }
      std::cout << "saved new node mobility." << std::endl;
      Ptr<Node> node = *j;
      Ptr<MobilityModel> mobility = node->GetObject<MobilityModel> ();
      osf << node->GetId() << "," << Simulator::Now();
      osf << "," << mobility->GetPosition() << "," << mobility->GetVelocity() << std::endl;
  }
  osf.close();
}

NS_LOG_COMPONENT_DEFINE ("SimpleSdnCarSwitchMmWave");

int
main (int argc, char *argv[])
{
  uint16_t simTime = 50; //45
  bool verbose = true;  // OpenFlow verbose
  bool trace = false;
  // Lte
  // bool useCa = false;
  // mm Wave Lte
  bool harqEnabled = true;
	bool rlcAmEnabled = true; //true to avoid msg="Error: cannot add the same kind of tag twice." when the controller and OF switch try to establish a connection
	bool fixedTti = false;
	unsigned symPerSf = 24;
	double sfPeriod = 100.0;
	unsigned run = 0;
	bool smallScale = true;
	double speed = 3;    // For doppler effect
  //802.11p
  std::string phyMode ("OfdmRate6MbpsBW10MHz");
  bool verbose11p = false;
  // Application
  bool v4pingVerbose = true;
  double interPacketInterval = 100;
  uint32_t payloadSize = 1472;                  /* Application payload size in bytes. */
  string dataRate = "100Kbps";                  /* Application data rate. */

  NS_UNUSED (interPacketInterval);
  NS_UNUSED (payloadSize);

  // Configure command line parameters
  CommandLine cmd;
  cmd.AddValue ("simTime", "Simulation time (seconds)", simTime);
  cmd.AddValue ("verbose", "Enable verbose output", verbose);
  cmd.AddValue ("trace", "Enable datapath stats and pcap traces", trace);
  // 802.11p
  cmd.AddValue ("phyMode", "Wifi Phy mode", phyMode);
  cmd.AddValue ("verbose11p", "turn on all WifiNetDevice log components", verbose11p);
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

  //This seems to have NO effect:
  // Config::SetDefault("ns3::LteEnbNetDevice::UlBandwidth", UintegerValue(50));
  // Config::SetDefault("ns3::LteEnbNetDevice::DlBandwidth", UintegerValue(50));
  // Config::SetDefault("ns3::CcHelper::UlBandwidth", UintegerValue(50));

  Config::SetDefault ("ns3::MmWave3gppChannel::UpdatePeriod", TimeValue(MilliSeconds(100)));    // interval after which the channel for a moving user is updated
  // Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::Scenario", StringValue("UMi-StreetCanyon"));
  Config::SetDefault ("ns3::MmWave3gppPropagationLossModel::Scenario", StringValue("RMa"));
  Config::SetDefault ("ns3::MmWaveHelper::ChannelModel", StringValue ("ns3::MmWave3gppChannel"));
  Config::SetDefault ("ns3::MmWaveHelper::PathlossModel", StringValue ("ns3::MmWave3gppPropagationLossModel"));

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

  Config::SetDefault ("ns3::QueueBase::MaxSize", QueueSizeValue (QueueSize ("500p")));

  if (verbose)
    {
      OFSwitch13Helper::EnableDatapathLogs ();
      //LogComponentEnable ("OFSwitch13Interface", LOG_LEVEL_ALL);
      LogComponentEnable ("OFSwitch13Device", LOG_LEVEL_ALL);
      LogComponentEnable ("OFSwitch13Port", LOG_LEVEL_INFO);
      // LogComponentEnable ("OFSwitch13Queue", LOG_LEVEL_ALL);
      //LogComponentEnable ("OFSwitch13SocketHandler", LOG_LEVEL_ALL);
      LogComponentEnable ("OFSwitch13Controller", LOG_LEVEL_ALL);
      // LogComponentEnable ("TrafficTypeController", LOG_LEVEL_ALL);
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
      // LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
      // LogComponentEnable ("StreamingApplication", LOG_INFO);
      // LogComponentEnable ("Ipv4RawSocketImpl", LOG_LEVEL_INFO);
      // LogComponentEnable ("Socket", LOG_LEVEL_INFO);
      // LogComponentEnable ("UdpSocketImpl", LOG_LEVEL_INFO);
      // LogComponentEnable ("UdpL4Protocol", LOG_LEVEL_INFO);
      // LogComponentEnable ("Ipv4L3Protocol", LOG_LEVEL_INFO);
      // LogComponentEnable ("Ipv4Interface", LOG_LEVEL_INFO);
      // LogComponentEnable ("ArpL3Protocol", LOG_LEVEL_INFO);
      // LogComponentEnable ("TrafficControlLayer", LOG_LEVEL_INFO);
      // LogComponentEnable ("QueueDisc", LOG_LEVEL_INFO);
      // LogComponentEnable ("CsmaNetDevice", LOG_LEVEL_INFO);
      // LogComponentEnable ("LteUeNetDevice", LOG_LEVEL_INFO);
      // LogComponentEnable ("LteNetDevice", LOG_LEVEL_INFO);
      // LogComponentEnable ("MmWaveNetDevice", LOG_LEVEL_INFO);
      // LogComponentEnable ("WifiNetDevice", LOG_LEVEL_INFO);
      // LogComponentEnable ("OcbWifiMac", LOG_LEVEL_INFO);
      // LogComponentEnable ("Queue", LOG_LOGIC);
      // LogComponentEnable ("PointToPointNetDevice", LOG_LEVEL_INFO);
    }

  // Enable checksum computations (required by OFSwitch13 module)
  GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));
  // To enable print packets
  ns3::PacketMetadata::Enable ();

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

  NS_LOG_INFO  ("pgw node id: " << pgw->GetId ());
  NS_LOG_INFO ("controller node id: " << controllerNode->GetId ());
  NS_LOG_INFO ("switch node id: " << switchNode->GetId ());

  NodeContainer ueNodes;
  NodeContainer lteEnbNodes;
  NodeContainer mmWaveEnbNodes;
  ueNodes.Add(controllerNode);
  ueNodes.Add(switchNode);
  lteEnbNodes.Create(1);
  mmWaveEnbNodes.Create(2);

  NS_LOG_INFO ("lte eNB node id: " << lteEnbNodes.Get (0)->GetId ());
  NS_LOG_INFO ("mmWave eNB node id: " << mmWaveEnbNodes.Get (0)->GetId () << " and " << mmWaveEnbNodes.Get (1)->GetId ());

  // Reserve Mac Address 00:00:00:00:00:01
  Mac48Address::Allocate();

  // Create an Internet Server
  NodeContainer remoteServerContainer;
  remoteServerContainer.Create(1);
  Ptr<Node> remoteServer = remoteServerContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteServerContainer);

  NS_LOG_INFO ("remote server node id: " << remoteServer->GetId ());

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

  /**** Create a 802.11p link ****/
  // NodeContainer wifi80211pNodes;
  // wifi80211pNodes.Create (1);                // We create the 802.11p host node.
  // wifi80211pNodes.Add(switchNode);           // The second node is also the OpenFlow Switch

  // The below set of helpers will help us to put together the wifi NICs we want
  // YansWifiPhyHelper wifiPhy =  YansWifiPhyHelper::Default ();
  // YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  // Ptr<YansWifiChannel> channel = wifiChannel.Create ();
  // wifiPhy.SetChannel (channel);
  //
  // wifiPhy.SetPcapDataLinkType (WifiPhyHelper::DLT_IEEE802_11);
  // NqosWaveMacHelper wifi80211pMac = NqosWaveMacHelper::Default ();
  // Wifi80211pHelper wifi80211p = Wifi80211pHelper::Default ();
  //
  // if (verbose11p)
  //   {
  //     wifi80211p.EnableLogComponents ();  // Turn on all Wifi 802.11p logging
  //   }
  //
  // wifi80211p.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
  //                                     "DataMode",StringValue (phyMode),
  //                                     "ControlMode",StringValue (phyMode));


  // // Install Mobility Model to LTE and mmWave nodes
  // MobilityHelper lteMobility;
  // Ptr<ListPositionAllocator> ltePositionAlloc = CreateObject<ListPositionAllocator> ();
  // // lte enb
  // ltePositionAlloc->Add (Vector(100, 400, 20));
  // // mm Wave enb
  // ltePositionAlloc->Add (Vector(196, 128, 20));
  // // Ue nodes
  // ltePositionAlloc->Add (Vector( 0, 500, 1)); //SDN Controller
  // // ltePositionAlloc->Add (Vector(100,156, 1)); //SDN Controller
  // ltePositionAlloc->Add (Vector(100,100, 1)); //Car with OpenFlow Switch
  // // Remote 802.11p node
  // ltePositionAlloc->Add (Vector(100,150, 1)); //Slightly north of the Car
  //
  // lteMobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  // lteMobility.SetPositionAllocator(ltePositionAlloc);
  // lteMobility.Install(lteEnbNodes);
  // lteMobility.Install(mmWaveEnbNodes);
  // lteMobility.Install(ueNodes);
  // lteMobility.Install(wifi80211pNodes.Get (0));

  Ns2MobilityHelper ns2mobility = Ns2MobilityHelper("sdnVanetSingle.movements");
  ns2mobility.Install();

  // Initialize file to store node's location
  ExportMobility ("start");
  Config::Connect ("/NodeList/*/$ns3::MobilityModel/CourseChange", MakeCallback (&CourseChange));

  // std::cout << "Controller location: " << ueNodes.Get(0)->GetObject<MobilityModel> ()->GetPosition() << std::endl;
  // std::cout << "Switch location: " << ueNodes.Get(1)->GetObject<MobilityModel> ()->GetPosition() << std::endl;
  // std::cout << "Lte Tower location: " << lteEnbNodes.Get(0)->GetObject<MobilityModel> ()->GetPosition() << std::endl;
  // std::cout << "Cellular mmWave Tower location: " << mmWaveEnbNodes.Get(0)->GetObject<MobilityModel> ()->GetPosition() << std::endl;

  NS_LOG_INFO ("Controller location:\t" << ueNodes.Get(0)->GetObject<MobilityModel> ()->GetPosition());
  NS_LOG_INFO ("Switch location:\t" << ueNodes.Get(1)->GetObject<MobilityModel> ()->GetPosition());
  NS_LOG_INFO ("Lte Tower location:\t" << lteEnbNodes.Get(0)->GetObject<MobilityModel> ()->GetPosition());
  NS_LOG_INFO ("Cellular mmWave Tower 1 location: " << mmWaveEnbNodes.Get(0)->GetObject<MobilityModel> ()->GetPosition());
  NS_LOG_INFO ("Cellular mmWave Tower 2 location: " << mmWaveEnbNodes.Get(1)->GetObject<MobilityModel> ()->GetPosition());
  // NS_LOG_INFO ("Remote 802.11p node location: " << wifi80211pNodes.Get(0)->GetObject<MobilityModel> ()->GetPosition());

  // Install LTE and mmWave Devices to the nodes
  NetDeviceContainer lteEnbDevs = mmWaveHelper->InstallLteEnbDevice (lteEnbNodes);
  NetDeviceContainer mmWaveEnbDevs = mmWaveHelper->InstallEnbDevice (mmWaveEnbNodes);
  // For an unknown reason, only by installing first the mmWave Ue device to the switch node, the simulation throws an error
  NetDeviceContainer mmWaveUeDevs = mmWaveHelper->InstallUeDevice ( NodeContainer(switchNode) );
  NetDeviceContainer lteUeDevs = mmWaveHelper->InstallLteUeDevice (ueNodes);
  // NetDeviceContainer lteUeDevs = mmWaveHelper->InstallMcUeDevice (ueNodes);
  // NetDeviceContainer lteUeDevs = mmWaveHelper->InstallMcUeDevice (NodeContainer(switchNode));

  // Install one more LTE device to the switch node
  lteUeDevs.Add (mmWaveHelper->InstallLteUeDevice ( NodeContainer(switchNode) ));
  // lteUeDevs.Add (mmWaveHelper->InstallMcUeDevice ( NodeContainer(switchNode) ));

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

  // Create the Wifi Network Devices
  // NetDeviceContainer wifi80211pDevs = wifi80211p.Install (wifiPhy, wifi80211pMac, wifi80211pNodes);
  // wifiPhy.EnablePcap ("wave-80211p", wifi80211pNodes);
  //
  // NS_LOG_INFO ("Local 802.11p interface: " << wifi80211pDevs.Get (1)->GetIfIndex ());
  // NS_LOG_INFO ("Local 802.11p interface: " << wifi80211pDevs.Get (1)->GetAddress ());
  // NS_LOG_INFO ("Remote 802.11p interface: " << wifi80211pDevs.Get (0)->GetAddress ());

  // Create three host nodes
  NodeContainer hosts;
  hosts.Create (3);

  // Use the CsmaHelper to connect host nodes to the switch node
  CsmaHelper csmaHelper;
  csmaHelper.SetChannelAttribute ("DataRate", DataRateValue (DataRate ("1000Mbps")));
  csmaHelper.SetChannelAttribute ("Delay", TimeValue (MicroSeconds (10)));
  NetDeviceContainer hostDevices;
  NetDeviceContainer switchPorts;
  for (size_t i = 0; i < hosts.GetN (); i++)
    {
      NodeContainer pair (hosts.Get (i), switchNode);
      NetDeviceContainer link = csmaHelper.Install (pair);
      hostDevices.Add (link.Get (0));
      switchPorts.Add (link.Get (1));
      NS_LOG_INFO ("New Host with CSMA Net Device address: " << link.Get(0)->GetAddress ());
      NS_LOG_INFO ("attached to switch port with index: " << link.Get(1)->GetIfIndex ());
    }
  // Fourth switch port is the LTE Net Device
  switchPorts.Add (lteUeDevs.Get (2));
  // Fifth switch port is the mm Wave Net Device
  switchPorts.Add (mmWaveUeDevs.Get (0));
  // Sixth port is the 802.11p device
  // switchPorts.Add (wifi80211pDevs.Get (1));

  // Configure the OpenFlow network domain
  Ptr<OFSwitch13InternalHelper> of13Helper = CreateObject<OFSwitch13InternalHelper> ();
  Ptr<TrafficTypeController> vanetCtrl = CreateObject<TrafficTypeController> ();
  of13Helper->InstallController (controllerNode, vanetCtrl);
  of13Helper->InstallSwitch (switchNode, switchPorts);
  /* of13Helper->CreateOpenFlowChannels (); */
  of13Helper->GetCtlDevs().Add(lteUeDevs.Get(0));

  // Assign IP address to LTE UEs and attach them to eNBs
  Ipv4InterfaceContainer ueIpIface;
  // ueIpIface = epcHelper->AssignUeIpv4Address (lteUeDevs);
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (lteUeDevs, mmWaveUeDevs));
  // ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (mmWaveUeDevs));
  for (size_t i = 0; i < ueIpIface.GetN (); i++ )
    {
      // std::cout << "Lte UE Device address: " << (*it)->GetAddress () << std::endl;
      NS_LOG_INFO ("UE Device IP address:\t" << ueIpIface.GetAddress (i));
    }
  //TODO check to see if this is needed in this test case where lte devs are installed in te Open Flow Switch:
  // for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
  //   {
  //     Ptr<Node> ueNode = ueNodes.Get (u);
  //     // Set the default gateway for the UE
  //     Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
  //     ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
  //   }

  // Attach UEs to eNB nodes
  /* UEs need to have IPv4/IPv6 installed before EPS bearers can be activated
     side effect: the default EPS bearer will be activated */
  mmWaveHelper->AttachToClosestLteEnb (lteUeDevs, lteEnbDevs);
  // mmWaveHelper->AttachToClosestEnb (lteUeDevs, mmWaveEnbDevs, lteEnbDevs);
  mmWaveHelper->AttachToClosestEnb (mmWaveUeDevs, mmWaveEnbDevs);

  // Start the connections between SDN controller and switches.
  UintegerValue portValue;
  of13Helper->GetCtlApps().Get(0)->GetAttribute ("Port", portValue);
  // Controller Application Socket Address
  InetSocketAddress addr (ueIpIface.GetAddress (0), portValue.Get ());
  uint64_t dpId = of13Helper->GetOFDevs().Get(0)->GetDatapathId ();
  NS_LOG_INFO ("Connecting switch " <<  dpId <<
               " to controller " << addr.GetIpv4 () << " port " << addr.GetPort ());

  Simulator::ScheduleNow (&OFSwitch13Device::StartControllerConnection, of13Helper->GetOFDevs().Get(0), addr);

  // Install the TCP/IP stack into remote 802.11p node
  // internet.Install (wifi80211pNodes.Get (0));
  // Install the TCP/IP stack into hosts nodes
  internet.Install (hosts);

  // Set wifi IPv4 addresses
  // Ipv4AddressHelper wifiIpv4helpr;
  // Ipv4InterfaceContainer wifiIpIfaces;
  // wifiIpv4helpr.SetBase ("20.1.1.0", "255.255.255.0", "0.0.0.1");
  // wifiIpIfaces = wifiIpv4helpr.Assign (wifi80211pDevs);
  //
  // NS_LOG_INFO ("Ipv4 address of ofswitch's port 6 (802.11p): " << switchNode->GetObject<Ipv4>()->GetAddress(4,0).GetLocal());

  // Set host IPv4 2addresses
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

  // Configure ping application
  // V4PingHelper pingHelper1 = V4PingHelper (wifiIpIfaces.GetAddress (0));
  // V4PingHelper pingHelper1 = V4PingHelper (remoteServerAddr);
  // pingHelper1.SetAttribute ("Verbose", BooleanValue (true));
  // V4PingHelper pingHelper2 = V4PingHelper (hostIpIfaces.GetAddress (1));
  // pingHelper2.SetAttribute ("Verbose", BooleanValue (true));

  // ApplicationContainer pingApps = pingHelper1.Install (hosts.Get (0));
  // pingApps.Add(pingHelper2.Install (hosts.Get (2)));

  // pingApps.Start (Seconds (3.9));

  /* UDP Data */
  uint16_t dlPort = 1000;
  uint16_t ulPort = 2000;
  NS_UNUSED (dlPort);
  ApplicationContainer serverApps;
  ApplicationContainer clientApps;

  /* Server Apps */
  StreamingHelper streamServer ("ns3::UdpSocketFactory", StreamingApplication::SERVER, InetSocketAddress (Ipv4Address::GetAny (), ulPort));
  streamServer.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  streamServer.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
  streamServer.SetAttribute ("RequestInterval", TimeValue (Seconds (0.375)));
  // serverApps.Add (streamServer.Install (remoteServer));

  // packetSinkServer = StaticCast<StreamingApplication> (serverApps.Get (0));

  /* Client Apps */
  StreamingHelper udpClient ("ns3::UdpSocketFactory", StreamingApplication::CLIENT, InetSocketAddress (remoteServerAddr, ulPort));
  udpClient.SetAttribute ("Local", AddressValue (InetSocketAddress (Ipv4Address::GetAny (), dlPort)));
  udpClient.SetAttribute ("RequestInterval", TimeValue (Seconds (0.25)));
  // clientApps.Add (udpClient.Install (hosts.Get (2)));

  // packetSinkHost = StaticCast<StreamingApplication> (clientApps.Get (0));

  // serverApps.Start (Seconds (0.1));
  // clientApps.Start (Seconds (3.9));

  // clientApps.Stop (Seconds (5.0));
  // Simulator::Schedule (Seconds (5.0), &StreamingApplication::StopSending, packetSinkHost);
  // Simulator::Schedule (Seconds (4), &CalculateThroughput);

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
  // After simulation ends, log final node's location to file
  ExportMobility ("end");
  Simulator::Destroy ();
}
