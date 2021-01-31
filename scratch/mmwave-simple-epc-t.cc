 /* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
 /*
 *   Copyright (c) 2011 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *   Copyright (c) 2015, NYU WIRELESS, Tandon School of Engineering, New York University
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2 as
 *   published by the Free Software Foundation;
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *   Author: Marco Miozzo <marco.miozzo@cttc.es>
 *           Nicola Baldo  <nbaldo@cttc.es>
 *
 *   Modified by: Marco Mezzavilla < mezzavilla@nyu.edu>
 *        	 	  Sourjya Dutta <sdutta@nyu.edu>
 *        	 	  Russell Ford <russell.ford@nyu.edu>
 *        		  Menglei Zhang <menglei@nyu.edu>
 */


#include "ns3/mmwave-helper.h"
#include "ns3/epc-helper.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/applications-module.h"
#include "ns3/point-to-point-helper.h"
#include "ns3/config-store.h"
#include "ns3/mmwave-point-to-point-epc-helper.h"
//#include "ns3/gtk-config-store.h"

using namespace ns3;
using namespace mmwave;
using namespace std;

/**
 * Sample simulation script for LTE+EPC. It instantiates several eNodeB,
 * attaches one UE per eNodeB starts a flow for each UE to  and from a remote host.
 * It also starts yet another flow between each UE pair.
 */

/**  Application Variables **/
uint64_t totalRx = 0;
double throughput = 0;
Ptr<PacketSink> packetSink;

NS_LOG_COMPONENT_DEFINE ("DualUeDevice");

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
  double thr = CalculateSingleStreamThroughput (packetSink, totalRx, throughput);
  std::cout << Simulator::Now ().GetSeconds () << "\t\t" << thr << std::endl;
  Simulator::Schedule (MilliSeconds (100), &CalculateThroughput);
}

int
main (int argc, char *argv[])
{
	//LogComponentEnable ("LteUeRrc", LOG_LEVEL_ALL);
	//LogComponentEnable ("LteEnbRrc", LOG_LEVEL_ALL);
//	LogComponentEnable("MmWavePointToPointEpcHelper",LOG_LEVEL_ALL);
//	LogComponentEnable("EpcUeNas",LOG_LEVEL_ALL);
//	LogComponentEnable ("MmWaveSpectrumPhy", LOG_LEVEL_LOGIC);
	//LogComponentEnable ("MmWaveUePhy", LOG_LEVEL_DEBUG);
	//LogComponentEnable ("MmWaveEnbPhy", LOG_LEVEL_DEBUG);
//	LogComponentEnable ("MmWaveUeMac", LOG_LEVEL_LOGIC);
	//LogComponentEnable ("UdpClient", LOG_LEVEL_INFO);
	//LogComponentEnable ("PacketSink", LOG_LEVEL_INFO);
	  //LogComponentEnable("PropagationLossModel",LOG_LEVEL_ALL);


	uint16_t numEnb = 1;
	uint16_t numUe = 1;
	double simTime = 2.0;
	double interPacketInterval = 100;  // 500 microseconds
	double minDistance = 10.0;  // eNB-UE distance in meters
	double maxDistance = 150.0;  // eNB-UE distance in meters
	bool harqEnabled = true;
	bool rlcAmEnabled = false;
	bool fixedTti = false;
	unsigned symPerSf = 24;
	double sfPeriod = 100.0;
	unsigned run = 0;
	bool smallScale = true;
	double speed = 3;

	uint32_t payloadSize = 1472;                  /* Application payload size in bytes. */
  string dataRate = "300Mbps";                  /* Application data rate. */

  NS_UNUSED (minDistance);
	NS_UNUSED (maxDistance);
	// Command line arguments
	CommandLine cmd;
	cmd.AddValue("numEnb", "Number of eNBs", numEnb);
	cmd.AddValue("numUe", "Number of UEs per eNB", numUe);
	cmd.AddValue("simTime", "Total duration of the simulation [s])", simTime);
	cmd.AddValue("interPacketInterval", "Inter-packet interval [us])", interPacketInterval);
	cmd.AddValue("harq", "Enable Hybrid ARQ", harqEnabled);
	cmd.AddValue("rlcAm", "Enable RLC-AM", rlcAmEnabled);
	cmd.AddValue("symPerSf", "OFDM symbols per subframe", symPerSf);
	cmd.AddValue("sfPeriod", "Subframe period = 4.16 * symPerSf", sfPeriod);
	cmd.AddValue("fixedTti", "Fixed TTI scheduler", fixedTti);
	cmd.AddValue("run", "run for RNG (for generating different deterministic sequences for different drops)", fixedTti);
	cmd.Parse(argc, argv);

  Config::SetDefault ("ns3::MmWaveHelper::ChannelModel", StringValue ("ns3::MmWaveBeamforming"));

  // Config::SetDefault ("ns3::MmWaveHelper::ChannelModel", StringValue ("ns3::MmWaveChannelMatrix"));

  // Config::SetDefault ("ns3::MmWaveHelper::ChannelModel", StringValue ("ns3::MmWave3gppChannel"));
  // Config::SetDefault ("ns3::MmWaveHelper::PathlossModel", StringValue ("ns3::MmWave3gppPropagationLossModel"));

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
	//Config::SetDefault ("ns3::MmWavePropagationLossModel::ChannelStates", StringValue ("n"));
	Config::SetDefault ("ns3::LteRlcAm::ReportBufferStatusTimer", TimeValue(MicroSeconds(100.0)));
	Config::SetDefault ("ns3::LteRlcUmLowLat::ReportBufferStatusTimer", TimeValue(MicroSeconds(100.0)));
	Config::SetDefault ("ns3::LteEnbRrc::SrsPeriodicity", UintegerValue (320));
	Config::SetDefault ("ns3::LteEnbRrc::FirstSibTime", UintegerValue (2));
	Config::SetDefault ("ns3::MmWaveBeamforming::SmallScaleFading", BooleanValue (smallScale));
	Config::SetDefault ("ns3::MmWaveBeamforming::FixSpeed", BooleanValue (true));
	Config::SetDefault ("ns3::MmWaveBeamforming::UeSpeed", DoubleValue (speed));

  RngSeedManager::SetSeed (1234);
	RngSeedManager::SetRun (run);

  Ptr<MmWaveHelper> mmwaveHelper = CreateObject<MmWaveHelper> ();
	mmwaveHelper->SetSchedulerType ("ns3::MmWaveFlexTtiMacScheduler");
  Ptr<MmWavePointToPointEpcHelper>  epcHelper = CreateObject<MmWavePointToPointEpcHelper> ();
  mmwaveHelper->SetEpcHelper (epcHelper);
	mmwaveHelper->SetHarqEnabled (harqEnabled);

  ConfigStore inputConfig;
  inputConfig.ConfigureDefaults();

  // parse again so you can override default values from the command line
  cmd.Parse(argc, argv);

  Ptr<Node> pgw = epcHelper->GetPgwNode ();

   // Create a single RemoteHost
  NodeContainer remoteHostContainer;
  remoteHostContainer.Create (1);
  Ptr<Node> remoteHost = remoteHostContainer.Get (0);
  InternetStackHelper internet;
  internet.Install (remoteHostContainer);

  // Create the Internet
  PointToPointHelper p2ph;
  p2ph.SetDeviceAttribute ("DataRate", DataRateValue (DataRate ("100Gb/s")));
  p2ph.SetDeviceAttribute ("Mtu", UintegerValue (1500));
  p2ph.SetChannelAttribute ("Delay", TimeValue (Seconds (0.010)));
  NetDeviceContainer internetDevices = p2ph.Install (pgw, remoteHost);
  Ipv4AddressHelper ipv4h;
  ipv4h.SetBase ("1.0.0.0", "255.0.0.0");
  Ipv4InterfaceContainer internetIpIfaces = ipv4h.Assign (internetDevices);
  // interface 0 is localhost, 1 is the p2p device
  Ipv4Address remoteHostAddr = internetIpIfaces.GetAddress (1);

  Ipv4StaticRoutingHelper ipv4RoutingHelper;
  Ptr<Ipv4StaticRouting> remoteHostStaticRouting = ipv4RoutingHelper.GetStaticRouting (remoteHost->GetObject<Ipv4> ());
  remoteHostStaticRouting->AddNetworkRouteTo (Ipv4Address ("7.0.0.0"), Ipv4Mask ("255.0.0.0"), 1);

  NodeContainer ueNodes;
  NodeContainer enbNodes;
	NodeContainer lteEnbNodes;
	NodeContainer lteUeNodes;
	lteUeNodes.Create (1);
	lteEnbNodes.Create(1);
  enbNodes.Create(numEnb);
  ueNodes.Create(numUe);

  // Install Mobility Model
  MobilityHelper lteEnbMobility;
	Ptr<ListPositionAllocator> lteEnbPositionAlloc = CreateObject<ListPositionAllocator> ();
	lteEnbPositionAlloc->Add (Vector (0.0, 0.0, 20.0));
	lteEnbMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	lteEnbMobility.SetPositionAllocator(lteEnbPositionAlloc);
	lteEnbMobility.Install (lteEnbNodes);

  Ptr<ListPositionAllocator> enbPositionAlloc = CreateObject<ListPositionAllocator> ();
  enbPositionAlloc->Add (Vector (100.0, 0.0, 20.0));
  MobilityHelper enbmobility;
  enbmobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  enbmobility.SetPositionAllocator(enbPositionAlloc);
  enbmobility.Install (enbNodes);

	MobilityHelper lteUeMobility;
	Ptr<ListPositionAllocator> lteUePositionAlloc = CreateObject<ListPositionAllocator> ();
	lteUePositionAlloc->Add (Vector (700.0, 0.0, 1.0));
	lteUeMobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
	lteUeMobility.SetPositionAllocator(lteUePositionAlloc);
	lteUeMobility.Install (lteUeNodes);

  MobilityHelper uemobility;
  Ptr<ListPositionAllocator> uePositionAlloc = CreateObject<ListPositionAllocator> ();
	// Ptr<UniformRandomVariable> distRv = CreateObject<UniformRandomVariable> ();
	// for (unsigned i = 0; i < numUe; i++)
	// {
	// 	double dist = distRv->GetValue (minDistance, maxDistance);
	// 	uePositionAlloc->Add (Vector (dist, 0.0, 0.0));
	// }
	uePositionAlloc->Add (Vector (100.0+110, 0.0, 1.0));
	uemobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  uemobility.SetPositionAllocator(uePositionAlloc);
  uemobility.Install (ueNodes);

  // Install mmWave Devices to the nodes
	NetDeviceContainer lteEnbDevs = mmwaveHelper->InstallLteEnbDevice (lteEnbNodes);
  NetDeviceContainer enbmmWaveDevs = mmwaveHelper->InstallEnbDevice (enbNodes);
  NetDeviceContainer uemmWaveDevs = mmwaveHelper->InstallUeDevice (ueNodes);
	NetDeviceContainer lteUeDevs = mmwaveHelper->InstallMcUeDevice (lteUeNodes);
  // NetDeviceContainer lteUeDevs = mmwaveHelper->InstallLteUeDevice (lteUeNodes);

  // Install the IP stack on the UEs
  internet.Install (ueNodes);
	internet.Install (lteUeNodes);
  Ipv4InterfaceContainer ueIpIface;
  ueIpIface = epcHelper->AssignUeIpv4Address (NetDeviceContainer (uemmWaveDevs, lteUeDevs));
  // Assign IP address to UEs, and install applications
  for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
    {
      Ptr<Node> ueNode = ueNodes.Get (u);
      // Set the default gateway for the UE
      Ptr<Ipv4StaticRouting> ueStaticRouting = ipv4RoutingHelper.GetStaticRouting (ueNode->GetObject<Ipv4> ());
      ueStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);
    }

	Ptr<Ipv4StaticRouting> lteUeStaticRouting = ipv4RoutingHelper.GetStaticRouting (lteUeNodes.Get(0)->GetObject<Ipv4> ());
	lteUeStaticRouting->SetDefaultRoute (epcHelper->GetUeDefaultGatewayAddress (), 1);

  mmwaveHelper->AttachToClosestEnb (uemmWaveDevs, enbmmWaveDevs);
	mmwaveHelper->AttachToClosestLteEnb (lteUeDevs, lteEnbDevs);


  // Install and start applications on UEs and remote host
  uint16_t dlPort = 1234;
  uint16_t ulPort = 2000;
  uint16_t otherPort = 3000;
  ApplicationContainer clientApps;
  ApplicationContainer serverApps;
	ApplicationContainer srcApp;

	NS_UNUSED (dlPort);
  NS_UNUSED (otherPort);

//   for (uint32_t u = 0; u < ueNodes.GetN (); ++u)
//     {
//       ++ulPort;
//       ++otherPort;
//       // PacketSinkHelper dlPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), dlPort));
//       PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ulPort));
//       // PacketSinkHelper packetSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), otherPort));
//       // serverApps.Add (dlPacketSinkHelper.Install (ueNodes.Get(u)));
//       serverApps.Add (ulPacketSinkHelper.Install (remoteHost));
//       // serverApps.Add (packetSinkHelper.Install (ueNodes.Get(u)));
// 			packetSink = StaticCast<PacketSink> (serverApps.Get (0));
//
//       // UdpClientHelper dlClient (ueIpIface.GetAddress (u), dlPort);
//       // dlClient.SetAttribute ("Interval", TimeValue (MicroSeconds(interPacketInterval)));
//       // dlClient.SetAttribute ("MaxPackets", UintegerValue(1000000));
// 			//
//       // UdpClientHelper ulClient (remoteHostAddr, ulPort);
//       // ulClient.SetAttribute ("Interval", TimeValue (MicroSeconds(interPacketInterval)));
//       // ulClient.SetAttribute ("MaxPackets", UintegerValue(1000000));
//
// 			/* UDP Transmitter */
// 			OnOffHelper src ("ns3::UdpSocketFactory", InetSocketAddress (remoteHostAddr, ulPort));
// 			src.SetAttribute ("MaxBytes", UintegerValue (0));
// 			src.SetAttribute ("PacketSize", UintegerValue (payloadSize));
// 			src.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1e6]"));
// 			src.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
// 			src.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
//
// //      UdpClientHelper client (ueIpIface.GetAddress (u), otherPort);
// //      client.SetAttribute ("Interval", TimeValue (MilliSeconds(interPacketInterval)));
// //      client.SetAttribute ("MaxPackets", UintegerValue(1000000));
//
//       // clientApps.Add (dlClient.Install (remoteHost));
//       // clientApps.Add (ulClient.Install (ueNodes.Get(u)));
// 			srcApp = src.Install(ueNodes.Get(u));
//
// //      if (u+1 < ueNodes.GetN ())
// //        {
// //          clientApps.Add (client.Install (ueNodes.Get(u+1)));
// //        }
// //      else
// //        {
// //          clientApps.Add (client.Install (ueNodes.Get(0)));
// //        }
//     }

  /* LTE UE Device as souce of data */
  PacketSinkHelper ulPacketSinkHelper ("ns3::UdpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), ulPort));
  serverApps.Add (ulPacketSinkHelper.Install (remoteHost));
  packetSink = StaticCast<PacketSink> (serverApps.Get (0));
  /* UDP Transmitter */
  OnOffHelper src ("ns3::UdpSocketFactory", InetSocketAddress (remoteHostAddr, ulPort));
  src.SetAttribute ("MaxBytes", UintegerValue (0));
  src.SetAttribute ("PacketSize", UintegerValue (payloadSize));
  src.SetAttribute ("OnTime", StringValue ("ns3::ConstantRandomVariable[Constant=1e6]"));
  src.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));
  src.SetAttribute ("DataRate", DataRateValue (DataRate (dataRate)));
  srcApp = src.Install(lteUeNodes.Get(0));

  serverApps.Start (Seconds (0.01));
  // clientApps.Start (Seconds (0.01));
  srcApp.Start (Seconds (1.0));

  // mmwaveHelper->EnableTraces ();
  // Uncomment to enable PCAP tracing
  // p2ph.EnablePcapAll("mmwave-epc-simple");

	/* Print Output*/
  std::cout << "Time [s]" << '\t' << "Throughput [Mbps]" << std::endl;

  /* Schedule Throughput Calulcations */
  Simulator::Schedule (Seconds (1.1), &CalculateThroughput);

  Simulator::Stop(Seconds(simTime + 0.1));
  Simulator::Run();

  /*GtkConfigStore config;
  config.ConfigureAttributes();*/

  Simulator::Destroy();
  return 0;

}
