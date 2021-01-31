/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
//
// Copyright (c) 2020 Florida International University, ADWISE Laboratory
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation;
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Author: Oscar Bautista <obaut004@fiu.edu>
// Based on the OnOff Application by George F. Riley<riley@ece.gatech.edu>
// and the PacketSink Application by Tom Henderson (tomhend@u.washington.edu)
//

#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/packet-socket-address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "streaming-application.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/enum.h"
#include "ns3/stats-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("StreamingApplication");

NS_OBJECT_ENSURE_REGISTERED (StreamingApplication);

TypeId
StreamingApplication::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::StreamingApplication")
    .SetParent<Application> ()
    .SetGroupName("Applications")
    .AddConstructor<StreamingApplication> ()
    .AddAttribute ("DataRate", "The data rate in on state.",
                   DataRateValue (DataRate ("500kb/s")),
                   MakeDataRateAccessor (&StreamingApplication::m_cbrRate),
                   MakeDataRateChecker ())
    .AddAttribute ("PacketSize", "The size of packets sent in on state",
                   UintegerValue (512),
                   MakeUintegerAccessor (&StreamingApplication::m_pktSize),
                   MakeUintegerChecker<uint32_t> (1))
    .AddAttribute ("Remote", "The address of the destination host",
                   AddressValue (),
                   MakeAddressAccessor (&StreamingApplication::m_peer),
                   MakeAddressChecker ())
    .AddAttribute ("Local",
                   "The Local Address on which to Bind the socket.",
                   // Necessary for the case when in client mode a local address is not specified
                   AddressValue (InetSocketAddress (Ipv4Address::GetAny (), 0)),
                   MakeAddressAccessor (&StreamingApplication::m_local),
                   MakeAddressChecker ())
    .AddAttribute ("MaxBytes",
                   "The total number of bytes to send. Once these bytes are sent, "
                   "no packet is sent again, even in on state. The value zero means "
                   "that there is no limit.",
                   UintegerValue (0),
                   MakeUintegerAccessor (&StreamingApplication::m_maxBytes),
                   MakeUintegerChecker<uint64_t> ())
    .AddAttribute ("Protocol", "The type of protocol to use. This should be "
                   "a subclass of ns3::SocketFactory",
                   TypeIdValue (UdpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor (&StreamingApplication::m_tid),
                   // This should check for SocketFactory as a parent
                   MakeTypeIdChecker ())
    .AddAttribute ("Role", "The behavior of the application as a client or server",
                   EnumValue (StreamingApplication::CLIENT),
                   MakeEnumAccessor (&StreamingApplication::m_hostType),
                   MakeEnumChecker (StreamingApplication::SERVER, "Server",
                                    StreamingApplication::CLIENT, "Client"))
    .AddAttribute ("RequestInterval",
                   "Data Request Interval",
                   TimeValue (Seconds (0.5)),
                   MakeTimeAccessor (&StreamingApplication::m_requestInterval),
                   MakeTimeChecker ())
    .AddTraceSource ("Tx", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&StreamingApplication::m_txTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("TxWithAddresses", "A new packet is created and is sent",
                     MakeTraceSourceAccessor (&StreamingApplication::m_txTraceWithAddresses),
                     "ns3::Packet::TwoAddressTracedCallback")
    .AddTraceSource ("Rx",
                     "A packet has been received",
                     MakeTraceSourceAccessor (&StreamingApplication::m_rxTrace),
                     "ns3::Packet::AddressTracedCallback")
    .AddTraceSource ("RxWithAddresses", "A packet has been received",
                     MakeTraceSourceAccessor (&StreamingApplication::m_rxTraceWithAddresses),
                     "ns3::Packet::TwoAddressTracedCallback")
  ;
  return tid;
}

StreamingApplication::StreamingApplication ()
  : m_socket (0),
    m_connected (false),
    m_sendEnabled (true),
    m_residualBits (0),
    m_lastStartTime (Seconds (0)),
    m_requestInterval (Seconds (0.5)),
    m_totalTx (0),
    m_totalRx (0),
    m_totalPacketsRx (0),
    m_packetsSent (0),
    m_totalPacketDelay (Seconds (0))
{
  NS_LOG_FUNCTION (this);
}

StreamingApplication::~StreamingApplication()
{
  NS_LOG_FUNCTION (this);
}

void
StreamingApplication::SetMaxBytes (uint64_t maxBytes)
{
  NS_LOG_FUNCTION (this << maxBytes);
  m_maxBytes = maxBytes;
}

void
StreamingApplication::StopSending (void)
{
  m_sendEnabled = false;
  if (m_sendEvent.IsRunning ())
    {
      m_sendEvent.Cancel ();
    }
}

uint64_t
StreamingApplication::GetTotalRx () const
{
  NS_LOG_FUNCTION (this);
  return m_totalRx;
}

uint64_t
StreamingApplication::GetTotalPacketsRx () const
{
  NS_LOG_FUNCTION (this);
  return m_totalPacketsRx;
}

Ptr<Socket>
StreamingApplication::GetSocket (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socket;
}

std::list<Ptr<Socket> >
StreamingApplication::GetAcceptedSockets (void) const
{
  NS_LOG_FUNCTION (this);
  return m_socketList;
}

void
StreamingApplication::DoDispose (void)
{
  NS_LOG_FUNCTION (this);

  m_socket = 0;
  // chain up
  Application::DoDispose ();
}

// Application Methods
void StreamingApplication::StartApplication () // Called at time specified by Start
{
  NS_LOG_FUNCTION (this);
  int localPort = 0;
  if (InetSocketAddress::IsMatchingType (m_local))
    localPort = InetSocketAddress::ConvertFrom (m_local).GetPort ();

  if ( m_hostType == SERVER)
    NS_ASSERT_MSG ( localPort !=0, "In server(0) mode, a local port number to bind the socket is required");

  // Create the socket if not already
  if (!m_socket)
    {
      int bindSuccess = -1;
      m_socket = Socket::CreateSocket (GetNode (), m_tid);
      if (InetSocketAddress::IsMatchingType (m_local) ||
          PacketSocketAddress::IsMatchingType (m_local))
        {
          if (!localPort)
            {
              bindSuccess = m_socket->Bind ();
              NS_LOG_INFO ("StreamingApplication in mode " << m_hostType << ": binding socket to ephemeral port.");
            }
          else
            {
              bindSuccess = m_socket->Bind (InetSocketAddress (Ipv4Address::GetAny (), localPort));
              NS_LOG_INFO ("StreamingApplication in mode " << m_hostType << ": binding socket to local port: " << localPort );
            }
        }
      if (bindSuccess == -1)
        NS_FATAL_ERROR ("Failed to bind socket");

      m_socket->Listen();

      if (m_hostType == CLIENT)
        {
          int connected = m_socket->Connect (m_peer);
          m_socket->SetConnectCallback (
            MakeCallback (&StreamingApplication::ConnectionSucceeded, this),
            MakeCallback (&StreamingApplication::ConnectionFailed, this));
          m_socket->SetRecvCallback (MakeCallback (&StreamingApplication::HandleClientRead, this));
          if (connected == 0)
            {
              NS_LOG_INFO ("StreamingApplication in Client(1) mode: successfully connected socket to address " <<
              InetSocketAddress::ConvertFrom(m_peer).GetIpv4 ()  <<  " port " <<
              InetSocketAddress::ConvertFrom(m_peer).GetPort () );
              NS_LOG_INFO ("StreamingApplication in Client(1) mode: at time " << Simulator::Now () << " ...sending first packet" );
              SendPacket ();
            }
          else
            NS_LOG_INFO ("StreamingApplication in Client(1) mode: failed socket connection to address " <<
            InetSocketAddress::ConvertFrom(m_peer).GetIpv4 ()  <<  " port " <<
            InetSocketAddress::ConvertFrom(m_peer).GetPort () );
        }
      else if (m_hostType == SERVER)
        {
          m_socket->SetRecvCallback (MakeCallback (&StreamingApplication::HandleServerRead, this));
          m_socket->SetAcceptCallback (
            MakeNullCallback<bool, Ptr<Socket>, const Address &> (),
            MakeCallback (&StreamingApplication::HandleAccept, this));
          m_socket->SetCloseCallbacks (
            MakeCallback (&StreamingApplication::HandlePeerClose, this),
            MakeCallback (&StreamingApplication::HandlePeerError, this));
        }
    }
}

void StreamingApplication::StopApplication () // Called at time specified by Stop
{
  NS_LOG_FUNCTION (this);
  CancelSendEvent ();
  Simulator::Cancel (m_stopEvent);
  if(m_socket != 0)
    {
      m_socket->Close ();
    }
  else
    {
      NS_LOG_WARN ("StreamingApplication found null socket to close in StopApplication");
    }
  while(!m_socketList.empty ()) //these are accepted sockets, close them
    {
      Ptr<Socket> acceptedSocket = m_socketList.front ();
      m_socketList.pop_front ();
      acceptedSocket->Close ();
    }
}

void StreamingApplication::CancelSendEvent ()
{
  NS_LOG_FUNCTION (this);

  if (m_sendEvent.IsRunning () && m_cbrRateFailSafe == m_cbrRate )
    { // Cancel the pending send packet event
      // Calculate residual bits since last packet sent
      Time delta (Simulator::Now () - m_lastStartTime);
      int64x64_t bits = delta.To (Time::S) * m_cbrRate.GetBitRate ();
      m_residualBits += bits.GetHigh ();
    }
  m_cbrRateFailSafe = m_cbrRate;
  Simulator::Cancel (m_sendEvent);
}

// Private helpers
void StreamingApplication::ScheduleNextTx ()
{
  NS_LOG_FUNCTION (this);
  if ( m_sendEvent.IsRunning () ) return;
  if (m_maxBytes == 0 || m_totalTx < m_maxBytes)
    {
      Time nextTime;
      if (m_hostType == SERVER)
        {
          uint32_t bits = m_pktSize * 8 - m_residualBits;
          NS_LOG_LOGIC ("bits = " << bits);
          nextTime = (Seconds (bits /
                               static_cast<double>(m_cbrRate.GetBitRate ()))); // Time till next packet
        }
      else if (m_hostType == CLIENT)
        {
          nextTime = m_requestInterval;
        }
      NS_LOG_LOGIC ("nextTime = " << nextTime);
      m_sendEvent = Simulator::Schedule (nextTime,
                                         &StreamingApplication::SendPacket, this);
      NS_LOG_DEBUG ("StreamingAppliction at " << Simulator::Now () << ": scheduled next packet transmission in: " << nextTime);
    }
  else
    { // All done, cancel any pending events
      StopApplication ();
    }
}

void StreamingApplication::SendPacket ()
{
  if (!m_sendEnabled) return;
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("StreamingApplication at " << Simulator::Now () << ": sending packet.");
  NS_ASSERT (m_sendEvent.IsExpired ());
  StatsHeader statsHeader;
  statsHeader.SetSeq (m_packetsSent);
  statsHeader.SetNodeId (GetNode ()->GetId ());
  statsHeader.SetRxAddress (m_peer);
  Ptr<Packet> packet = Create<Packet> (m_pktSize-(statsHeader.GetSerializedSize ())); //  the size of the packet minus the size of the statsHeader header
  packet->AddHeader (statsHeader);
  m_socket->Send (packet);
  m_txTrace (packet);
  m_totalTx += m_pktSize;
  m_packetsSent++;
  Address localAddress;
  m_socket->GetSockName (localAddress);
  if (InetSocketAddress::IsMatchingType (m_peer))
    {
      NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                   << "s streaming application sent packet #" << m_packetsSent << " with "
                   <<  packet->GetSize () << " bytes to "
                   << InetSocketAddress::ConvertFrom(m_peer).GetIpv4 ()
                   << " port " << InetSocketAddress::ConvertFrom (m_peer).GetPort ()
                   << " total Tx " << m_totalTx << " bytes");
      m_txTraceWithAddresses (packet, localAddress, InetSocketAddress::ConvertFrom (m_peer));
    }
  m_lastStartTime = Simulator::Now ();
  m_residualBits = 0;
  ScheduleNextTx ();
}

void StreamingApplication::HandleClientRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  Address localAddress;
  while ((packet = socket->RecvFrom (from)))
    {
      if (packet->GetSize () == 0)
        { //EOF
          break;
        }
      m_totalRx += packet->GetSize ();
      m_totalPacketsRx++;
      UpdatePacketDelay (packet);
      if (InetSocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                       << "s streaming client received "
                       <<  packet->GetSize () << " bytes from "
                       << InetSocketAddress::ConvertFrom(from).GetIpv4 ()
                       << " port " << InetSocketAddress::ConvertFrom (from).GetPort ()
                       << " total Rx " << m_totalRx << " bytes");
        }

      socket->GetSockName (localAddress);
      m_rxTrace (packet, from);
      m_rxTraceWithAddresses (packet, from, localAddress);
    }
}

void StreamingApplication::HandleServerRead (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  Ptr<Packet> packet;
  Address from;
  Address localAddress;
  while ((packet = socket->RecvFrom (from)))
    {
      if (packet->GetSize () == 0)
        {
          break;
        }
      m_totalRx += packet->GetSize ();
      m_totalPacketsRx++;
      UpdatePacketDelay (packet);
      if (InetSocketAddress::IsMatchingType (from))
        {
          NS_LOG_INFO ("At time " << Simulator::Now ().GetSeconds ()
                       << "s streaming server received "
                       <<  packet->GetSize () << " bytes from "
                       << InetSocketAddress::ConvertFrom(from).GetIpv4 ()
                       << " port " << InetSocketAddress::ConvertFrom (from).GetPort ()
                       << " total Rx " << m_totalRx << " bytes");
        }
      socket->GetSockName (localAddress);
      m_rxTrace (packet, from);
      m_rxTraceWithAddresses (packet, from, localAddress);

      if (!m_connected || (m_peer != from))
        {
          m_peer = from;
          NS_LOG_INFO ("StreamingApplication in Server(0) mode: at " << Simulator::Now () << ": connecting to " <<
           InetSocketAddress::ConvertFrom(m_peer).GetIpv4 () << " port " << InetSocketAddress::ConvertFrom (m_peer).GetPort ());
          int connected = m_socket->Connect (m_peer);
          m_socket->SetConnectCallback (
            MakeCallback (&StreamingApplication::ConnectionSucceeded, this),
            MakeCallback (&StreamingApplication::ConnectionFailed, this));
          if (connected == 0)
            {
              ScheduleNextTx ();
            }
        }
      if (m_stopEvent.IsRunning ())
        {
          m_stopEvent.Cancel ();
        }
      m_stopEvent = Simulator::Schedule (2*m_requestInterval, &StreamingApplication::CancelSendEvent, this);
      NS_LOG_DEBUG ("StreamingAppliction at " << Simulator::Now () << ": scheduled transmission stop in: " << 2*m_requestInterval);
    }
}

Time StreamingApplication::GetTotalDelay ()
{
  NS_LOG_FUNCTION (this);
  Time retvalue = m_totalPacketDelay;
  m_totalPacketDelay = Seconds (0);
  return retvalue;
}

void StreamingApplication::UpdatePacketDelay (Ptr<Packet> packet)
{
  NS_LOG_FUNCTION (this);
  Time currrentTime = Simulator::Now();
  StatsHeader statsHeader;
  packet->PeekHeader (statsHeader);

  Time packetSentTime = statsHeader.GetTs ();
  m_totalPacketDelay += currrentTime - packetSentTime;
}

void StreamingApplication::ConnectionSucceeded (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
  NS_LOG_INFO ("StreamingApplication in Client(1) mode: at " << Simulator::Now () << ": successfully connected socket to address " <<
  InetSocketAddress::ConvertFrom(m_peer).GetIpv4 ()  <<  " port " <<
  InetSocketAddress::ConvertFrom(m_peer).GetPort () );

  m_connected = true;
}

void StreamingApplication::ConnectionFailed (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}

void StreamingApplication::HandleAccept (Ptr<Socket> s, const Address& from)
{
  NS_LOG_FUNCTION (this << s << from);
  s->SetRecvCallback (MakeCallback (&StreamingApplication::HandleServerRead, this));
  m_socketList.push_back (s);
}

void StreamingApplication::HandlePeerClose (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}

void StreamingApplication::HandlePeerError (Ptr<Socket> socket)
{
  NS_LOG_FUNCTION (this << socket);
}

} // Namespace ns3
