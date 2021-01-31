/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
//
// Copyright (c) 2006 Georgia Tech Research Corporation
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

#ifndef STREAMING_APPLICATION_H
#define STREAMING_APPLICATION_H

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/data-rate.h"
#include "ns3/traced-callback.h"

namespace ns3 {

class Address;
class RandomVariableStream;
class Socket;

/**
 * \ingroup applications
 * \defgroup streaming StreamingApplication
 *
 * This traffic generator simulates streaming from a server.
 * After Application::StartApplication, the CLIENT sends periodic packets
 * that act as requests to the server. The SERVER then replies with a stream
 * of UDP data at the specified data rate. When a periodic
 * request is not received, the server switchs to the Off state.
 * During the "Off" state, no traffic is generated.
 * During the "On" state, cbr traffic is generated. This cbr traffic is
 * characterized by the specified "data rate" and "packet size".
 */
/**
* \ingroup streaming
*
* \brief Generate traffic to a single destination according to an
*        OnOff pattern.
*
* This traffic generator simulates streaming from a server.
* After Application::StartApplication, the CLIENT sends periodic packets
* that act as requests to the server. The SERVER then replies with a stream
* of UDP data at the specified data rate. When a periodic
* request is not received, the server switchs to the Off state.
* During the "Off" state, no traffic is generated.
* During the "On" state, cbr traffic is generated. This cbr traffic is
* characterized by the specified "data rate" and "packet size".
*
* Note:  When an application is started, the first packet transmission
* occurs _after_ a delay equal to (packet size/bit rate).  Note also,
* when an application transitions into an off state in between packet
* transmissions, the remaining time until when the next transmission
* would have occurred is cached and is used when the application starts
* up again.  Example:  packet size = 1000 bits, bit rate = 500 bits/sec.
* If the application is started at time 3 seconds, the first packet
* transmission will be scheduled for time 5 seconds (3 + 1000/500)
* and subsequent transmissions at 2 second intervals.  If the above
* application were instead stopped at time 4 seconds, and restarted at
* time 5.5 seconds, then the first packet would be sent at time 6.5 seconds,
* because when it was stopped at 4 seconds, there was only 1 second remaining
* until the originally scheduled transmission, and this time remaining
* information is cached and used to schedule the next transmission
* upon restarting.
*
* If the underlying socket type supports broadcast, this application
* will automatically enable the SetAllowBroadcast(true) socket option.
*/
class StreamingApplication : public Application
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  StreamingApplication ();

  virtual ~StreamingApplication();

  /**
   * \enum HostType
   * \brief Enumeration to identify this host as a client or server.
   */
  enum HostType {
    SERVER,
    CLIENT
  };

  /**
   * \brief Set the total number of bytes to send.
   *
   * Once these bytes are sent, no packet is sent again, even in on state.
   * The value zero means that there is no limit.
   *
   * \param maxBytes the total number of bytes to send
   */
  void SetMaxBytes (uint64_t maxBytes);

  /**
   * \return the total bytes received by the host
   */
  uint64_t GetTotalRx () const;

  /**
   * \return the total count of packets received by the host
   */
  uint64_t GetTotalPacketsRx () const;

  /**
   * \return the sum of the received packet's delay
   * since the last read, and restarts the time
   */
  Time GetTotalDelay (void);

  /**
   * \brief Reads the Rx packet timestamp to calculate the packet delay
   * and update the time keeper variable
   */
  void UpdatePacketDelay (Ptr <Packet> packet);

  /**
   * \brief Disable scheduling of new packet transmissions
   */
  void StopSending();

  /**
   * \brief Return a pointer to associated socket.
   * \return pointer to associated socket
   */
  Ptr<Socket> GetSocket (void) const;

 /**
  * \return list of pointers to accepted sockets
  */
  std::list<Ptr<Socket> > GetAcceptedSockets (void) const;

protected:
  virtual void DoDispose (void);
private:
  // inherited from Application base class.
  virtual void StartApplication (void);    // Called at time specified by Start
  virtual void StopApplication (void);     // Called at time specified by Stop

  //helpers
  /**
   * \brief Cancel all pending events.
   */
  void CancelSendEvent ();

  /**
   * \brief Send a packet
   */
  void SendPacket ();

  enum HostType   m_hostType;     //!< Set the application behavior as a client or a host
  Ptr<Socket>     m_socket;       //!< Associated socket
  std::list<Ptr<Socket> > m_socketList;  //!< the accepted sockets
  Address         m_peer;         //!< Peer address
  Address         m_local;        //!< Local address to bind to
  bool            m_connected;    //!< True if connected
  bool            m_sendEnabled;  //!< flag to allow/disallow traffic
  DataRate        m_cbrRate;      //!< Rate that data is generated
  DataRate        m_cbrRateFailSafe;     //!< Rate that data is generated (check copy)
  uint32_t        m_pktSize;      //!< Size of packets
  uint32_t        m_residualBits; //!< Number of generated, but not sent, bits
  Time            m_lastStartTime; //!< Time last packet sent
  Time            m_requestInterval;     //!< Interval for streaming client periodic requests
  uint64_t        m_maxBytes;     //!< Limit total number of bytes sent
  uint64_t        m_totalTx;      //!< Total bytes sent so far
  uint64_t        m_totalRx;      //!< Total bytes received
  uint64_t        m_totalPacketsRx;      //!< Total packets received
  uint64_t        m_packetsSent;  //!< Counter of packets sent
  Time            m_totalPacketDelay;    //!< Total delay of packets received
  EventId         m_stopEvent;    //!< Event id for next start or stop event
  EventId         m_sendEvent;    //!< Event id of pending "send packet" event
  TypeId          m_tid;          //!< Type of the socket used

  /// Traced Callback: transmitted packets.
  TracedCallback<Ptr<const Packet> > m_txTrace;

  /// Callbacks for tracing the packet Tx events, includes source and destination addresses
  TracedCallback<Ptr<const Packet>, const Address &, const Address &> m_txTraceWithAddresses;

  /// Traced Callback: received packets, source address.
  TracedCallback<Ptr<const Packet>, const Address &> m_rxTrace;

  /// Callback for tracing the packet Rx events, includes source and destination addresses
  TracedCallback<Ptr<const Packet>, const Address &, const Address &> m_rxTraceWithAddresses;

private:
  /**
   * \brief Schedule the next packet transmission
   */
  void ScheduleNextTx ();
  /**
   * \brief Handle a packet received by the application
   * \param socket the receiving socket
   */
  void HandleClientRead (Ptr<Socket> socket);
  /**
   * \brief Handle a packet received by the application
   * \param socket the receiving socket
   */
  void HandleServerRead (Ptr<Socket> socket);
  /**
   * \brief Handle a Connection Succeed event
   * \param socket the connected socket
   */
  void ConnectionSucceeded (Ptr<Socket> socket);
  /**
   * \brief Handle a Connection Failed event
   * \param socket the not connected socket
   */
  void ConnectionFailed (Ptr<Socket> socket);
  /**
   * \brief Handle an incoming connection
   * \param socket the incoming connection socket
   * \param from the address the connection is from
   */
  void HandleAccept (Ptr<Socket> socket, const Address& from);
  /**
   * \brief Handle an connection close
   * \param socket the connected socket
   */
  void HandlePeerClose (Ptr<Socket> socket);
  /**
   * \brief Handle an connection error
   * \param socket the connected socket
   */
  void HandlePeerError (Ptr<Socket> socket);
};

} // namespace ns3

#endif /* STREAMING_APPLICATION_H */
