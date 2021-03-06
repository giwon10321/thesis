/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 The Boeing Company
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
 * Authors:
 *  Gary Pei <guangyu.pei@boeing.com>
 *  kwong yin <kwong-sang.yin@boeing.com>
 *  Tom Henderson <thomas.r.henderson@boeing.com>
 *  Sascha Alexander Jopen <jopen@cs.uni-bonn.de>
 *  Erwan Livolant <erwan.livolant@inria.fr>
 */
#include "lr-wpan-mac.h"
#include "lr-wpan-csmaca.h"
#include "lr-wpan-mac-header.h"
#include "lr-wpan-mac-trailer.h"
#include <ns3/simulator.h>
#include <ns3/log.h>
#include <ns3/uinteger.h>
#include <ns3/node.h>
#include <ns3/packet.h>
#include <ns3/random-variable-stream.h>
#include <ns3/double.h>

#include <ns3/rng-seed-manager.h>

#include "rf-mac-type-tag.h"
#include "rf-mac-duration-tag.h"
#include "rf-mac-group-tag.h"

#undef NS_LOG_APPEND_CONTEXT
#define NS_LOG_APPEND_CONTEXT                                   \
  std::clog << "[address " << m_shortAddress << "] ";

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LrWpanMac");

NS_OBJECT_ENSURE_REGISTERED (LrWpanMac);

const uint32_t LrWpanMac::aMinMPDUOverhead = 9; // Table 85

TypeId
LrWpanMac::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LrWpanMac")
    .SetParent<Object> ()
    .SetGroupName ("LrWpan")
    .AddConstructor<LrWpanMac> ()
    .AddAttribute ("PanId", "16-bit identifier of the associated PAN",
                   UintegerValue (),
                   MakeUintegerAccessor (&LrWpanMac::m_macPanId),
                   MakeUintegerChecker<uint16_t> ())
    .AddTraceSource ("MacTxEnqueue",
                     "Trace source indicating a packet has been "
                     "enqueued in the transaction queue",
                     MakeTraceSourceAccessor (&LrWpanMac::m_macTxEnqueueTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("MacTxDequeue",
                     "Trace source indicating a packet has was "
                     "dequeued from the transaction queue",
                     MakeTraceSourceAccessor (&LrWpanMac::m_macTxDequeueTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("MacTx",
                     "Trace source indicating a packet has "
                     "arrived for transmission by this device",
                     MakeTraceSourceAccessor (&LrWpanMac::m_macTxTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("MacTxOk",
                     "Trace source indicating a packet has been "
                     "successfully sent",
                     MakeTraceSourceAccessor (&LrWpanMac::m_macTxOkTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("MacTxDrop",
                     "Trace source indicating a packet has been "
                     "dropped during transmission",
                     MakeTraceSourceAccessor (&LrWpanMac::m_macTxDropTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("MacPromiscRx",
                     "A packet has been received by this device, "
                     "has been passed up from the physical layer "
                     "and is being forwarded up the local protocol stack.  "
                     "This is a promiscuous trace,",
                     MakeTraceSourceAccessor (&LrWpanMac::m_macPromiscRxTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("MacRx",
                     "A packet has been received by this device, "
                     "has been passed up from the physical layer "
                     "and is being forwarded up the local protocol stack.  "
                     "This is a non-promiscuous trace,",
                     MakeTraceSourceAccessor (&LrWpanMac::m_macRxTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("MacRxDrop",
                     "Trace source indicating a packet was received, "
                     "but dropped before being forwarded up the stack",
                     MakeTraceSourceAccessor (&LrWpanMac::m_macRxDropTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("Sniffer",
                     "Trace source simulating a non-promiscuous "
                     "packet sniffer attached to the device",
                     MakeTraceSourceAccessor (&LrWpanMac::m_snifferTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("PromiscSniffer",
                     "Trace source simulating a promiscuous "
                     "packet sniffer attached to the device",
                     MakeTraceSourceAccessor (&LrWpanMac::m_promiscSnifferTrace),
                     "ns3::Packet::TracedCallback")
    .AddTraceSource ("MacStateValue",
                     "The state of LrWpan Mac",
                     MakeTraceSourceAccessor (&LrWpanMac::m_lrWpanMacState),
                     "ns3::TracedValueCallback::LrWpanMacState")
    .AddTraceSource ("MacState",
                     "The state of LrWpan Mac",
                     MakeTraceSourceAccessor (&LrWpanMac::m_macStateLogger),
                     "ns3::LrWpanMac::StateTracedCallback")
    .AddTraceSource ("MacSentPkt",
                     "Trace source reporting some information about "
                     "the sent packet",
                     MakeTraceSourceAccessor (&LrWpanMac::m_sentPktTrace),
                     "ns3::LrWpanMac::SentTracedCallback")
  ;
  return tid;
}

LrWpanMac::LrWpanMac ()
{

  // First set the state to a known value, call ChangeMacState to fire trace source.
  m_lrWpanMacState = MAC_IDLE;
  ChangeMacState (MAC_IDLE);

  m_macRxOnWhenIdle = true;
  m_macPanId = 0;
  m_associationStatus = ASSOCIATED;
  m_selfExt = Mac64Address::Allocate ();
  m_macPromiscuousMode = false;
  m_macMaxFrameRetries = 3;
  m_retransmission = 0;
  m_numCsmacaRetry = 0;
  m_txPkt = 0;

  m_slotTimeOfData = MicroSeconds (20);
  m_slotTimeOfEnergy = MicroSeconds (10);
  m_sifsOfData = MicroSeconds (10);
  m_sifsOfEnergy = MicroSeconds (5);
  m_difsOfData = MicroSeconds (50);
  m_difsOfEnergy = MicroSeconds (25);

  m_receivedEnergyFromFirstSlot = 0.0;
  m_receivedEnergyFromSecondSlot = 0.0;

  Ptr<UniformRandomVariable> uniformVar = CreateObject<UniformRandomVariable> ();
  uniformVar->SetAttribute ("Min", DoubleValue (0.0));
  uniformVar->SetAttribute ("Max", DoubleValue (255.0));
  // m_macDsn = SequenceNumber8 (uniformVar->GetValue ());
  m_macDsn = 1;
  m_shortAddress = Mac16Address ("00:00");
}

LrWpanMac::~LrWpanMac ()
{
}

void
LrWpanMac::DoInitialize ()
{
  if (m_macRxOnWhenIdle)
    {
      m_phy->PlmeSetTRXStateRequest (IEEE_802_15_4_PHY_RX_ON);
    }
  else
    {
      m_phy->PlmeSetTRXStateRequest (IEEE_802_15_4_PHY_TRX_OFF);
    }

  Object::DoInitialize ();
}

void
LrWpanMac::DoDispose ()
{
  if (m_csmaCa != 0)
    {
      m_csmaCa->Dispose ();
      m_csmaCa = 0;
    }
  m_txPkt = 0;
  for (uint32_t i = 0; i < m_txQueue.size (); i++)
    {
      m_txQueue[i]->txQPkt = 0;
      delete m_txQueue[i];
    }
  m_txQueue.clear ();
  m_phy = 0;
  m_mcpsDataIndicationCallback = MakeNullCallback< void, McpsDataIndicationParams, Ptr<Packet> > ();
  m_mcpsDataConfirmCallback = MakeNullCallback< void, McpsDataConfirmParams > ();

  Object::DoDispose ();
}

bool
LrWpanMac::GetRxOnWhenIdle ()
{
  return m_macRxOnWhenIdle;
}

void
LrWpanMac::SetRxOnWhenIdle (bool rxOnWhenIdle)
{
  NS_LOG_FUNCTION (this << rxOnWhenIdle);
  m_macRxOnWhenIdle = rxOnWhenIdle;

  if (m_lrWpanMacState == MAC_IDLE)
    {
      if (m_macRxOnWhenIdle)
        {
          m_phy->PlmeSetTRXStateRequest (IEEE_802_15_4_PHY_RX_ON);
        }
      else
        {
          m_phy->PlmeSetTRXStateRequest (IEEE_802_15_4_PHY_TRX_OFF);
        }
    }
}

void
LrWpanMac::SetShortAddress (Mac16Address address)
{
  //NS_LOG_FUNCTION (this << address);
  m_shortAddress = address;
}

void
LrWpanMac::SetExtendedAddress (Mac64Address address)
{
  //NS_LOG_FUNCTION (this << address);
  m_selfExt = address;
}


Mac16Address
LrWpanMac::GetShortAddress () const
{
  NS_LOG_FUNCTION (this);
  return m_shortAddress;
}

Mac64Address
LrWpanMac::GetExtendedAddress () const
{
  NS_LOG_FUNCTION (this);
  return m_selfExt;
}

void
LrWpanMac::McpsDataRequest (McpsDataRequestParams params, Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);

  McpsDataConfirmParams confirmParams;
  confirmParams.m_msduHandle = params.m_msduHandle;

  // TODO: We need a drop trace for the case that the packet is too large or the request parameters are maleformed.
  //       The current tx drop trace is not suitable, because packets dropped using this trace carry the mac header
  //       and footer, while packets being dropped here do not have them.

  LrWpanMacHeader macHdr (LrWpanMacHeader::LRWPAN_MAC_DATA, m_macDsn.GetValue ());
  m_macDsn++;

  if (p->GetSize () > LrWpanPhy::aMaxPhyPacketSize - aMinMPDUOverhead)
    {
      // Note, this is just testing maximum theoretical frame size per the spec
      // The frame could still be too large once headers are put on
      // in which case the phy will reject it instead
      NS_LOG_ERROR (this << " packet too big: " << p->GetSize ());
      confirmParams.m_status = IEEE_802_15_4_FRAME_TOO_LONG;
      if (!m_mcpsDataConfirmCallback.IsNull ())
        {
          m_mcpsDataConfirmCallback (confirmParams);
        }
      return;
    }

  if ((params.m_srcAddrMode == NO_PANID_ADDR)
      && (params.m_dstAddrMode == NO_PANID_ADDR))
    {
      NS_LOG_ERROR (this << " Can not send packet with no Address field" );
      confirmParams.m_status = IEEE_802_15_4_INVALID_ADDRESS;
      if (!m_mcpsDataConfirmCallback.IsNull ())
        {
          m_mcpsDataConfirmCallback (confirmParams);
        }
      return;
    }
  switch (params.m_srcAddrMode)
    {
    case NO_PANID_ADDR:
      macHdr.SetSrcAddrMode (params.m_srcAddrMode);
      macHdr.SetNoPanIdComp ();
      break;
    case ADDR_MODE_RESERVED:
      macHdr.SetSrcAddrMode (params.m_srcAddrMode);
      break;
    case SHORT_ADDR:
      macHdr.SetSrcAddrMode (params.m_srcAddrMode);
      macHdr.SetSrcAddrFields (GetPanId (), GetShortAddress ());
      break;
    case EXT_ADDR:
      macHdr.SetSrcAddrMode (params.m_srcAddrMode);
      macHdr.SetSrcAddrFields (GetPanId (), GetExtendedAddress ());
      break;
    default:
      NS_LOG_ERROR (this << " Can not send packet with incorrect Source Address mode = " << params.m_srcAddrMode);
      confirmParams.m_status = IEEE_802_15_4_INVALID_ADDRESS;
      if (!m_mcpsDataConfirmCallback.IsNull ())
        {
          m_mcpsDataConfirmCallback (confirmParams);
        }
      return;
    }

  macHdr.SetDstAddrMode (params.m_dstAddrMode);
  // TODO: Add field for EXT_ADDR destination address (and use it here).
  macHdr.SetDstAddrFields (params.m_dstPanId, params.m_dstAddr);
  macHdr.SetSecDisable ();
  //extract the last 3 bits in TxOptions and map to macHdr
  int b0 = params.m_txOptions & TX_OPTION_ACK;
  int b1 = params.m_txOptions & TX_OPTION_GTS;
  int b2 = params.m_txOptions & TX_OPTION_INDIRECT;
  if (b0 == TX_OPTION_ACK)
    {
      // Set AckReq bit only if the destination is not the broadcast address.
      if (!(macHdr.GetDstAddrMode () == SHORT_ADDR && macHdr.GetShortDstAddr () == "ff:ff"))
        {
          macHdr.SetAckReq ();
        }
    }
  else if (b0 == 0)
    {
      macHdr.SetNoAckReq ();
    }
  else
    {
      confirmParams.m_status = IEEE_802_15_4_INVALID_PARAMETER;
      NS_LOG_ERROR (this << "Incorrect TxOptions bit 0 not 0/1");
      if (!m_mcpsDataConfirmCallback.IsNull ())
        {
          m_mcpsDataConfirmCallback (confirmParams);
        }
      return;
    }

  //if is Slotted CSMA means its beacon enabled
  if (m_csmaCa->IsSlottedCsmaCa ())
    {
      if (b1 == TX_OPTION_GTS)
        {
          //TODO:GTS Transmission
        }
      else if (b1 == 0)
        {
          //TODO:CAP Transmission
        }
      else
        {
          NS_LOG_ERROR (this << "Incorrect TxOptions bit 1 not 0/1");
          confirmParams.m_status = IEEE_802_15_4_INVALID_PARAMETER;
          if (!m_mcpsDataConfirmCallback.IsNull ())
            {
              m_mcpsDataConfirmCallback (confirmParams);
            }
          return;
        }
    }
  else
    {
      if (b1 != 0)
        {
          NS_LOG_ERROR (this << "for non-beacon-enables PAN, bit 1 should always be set to 0");
          confirmParams.m_status = IEEE_802_15_4_INVALID_PARAMETER;
          if (!m_mcpsDataConfirmCallback.IsNull ())
            {
              m_mcpsDataConfirmCallback (confirmParams);
            }
          return;
        }
    }

  if (b2 == TX_OPTION_INDIRECT)
    {
      //TODO :indirect tx
    }
  else if (b2 == 0)
    {
      //TODO :direct tx
    }
  else
    {
      NS_LOG_ERROR (this << "Incorrect TxOptions bit 2 not 0/1");
      confirmParams.m_status = IEEE_802_15_4_INVALID_PARAMETER;
      if (!m_mcpsDataConfirmCallback.IsNull ())
        {
          m_mcpsDataConfirmCallback (confirmParams);
        }
      return;
    }

  RfMacTypeTag typeTag;
  typeTag.Set (RfMacTypeTag::RF_MAC_DATA);
  p->AddPacketTag (typeTag);

  p->AddHeader (macHdr);

  LrWpanMacTrailer macTrailer;
  // Calculate FCS if the global attribute ChecksumEnable is set.
  if (Node::ChecksumEnabled ())
    {
      macTrailer.EnableFcs (true);
      macTrailer.SetFcs (p);
    }
  p->AddTrailer (macTrailer);

  m_macTxEnqueueTrace (p);

  TxQueueElement *txQElement = new TxQueueElement;
  txQElement->txQMsduHandle = params.m_msduHandle;
  txQElement->txQPkt = p;
  m_txQueue.push_back (txQElement);

  CheckQueue ();
  // m_rfMacTimer = Simulator::Schedule (GetDifsOfData (), &LrWpanMac::CheckQueue, this);
}

void
LrWpanMac::CheckQueue ()
{
  NS_LOG_FUNCTION (this << !m_txQueue.empty () << " " << m_txPkt << " " <<!m_setMacState.IsRunning ());
  // Pull a packet from the queue and start sending, if we are not already sending.
  if (m_lrWpanMacState == MAC_IDLE && !m_txQueue.empty () && m_txPkt == 0 && !m_setMacState.IsRunning ())
    {
      TxQueueElement *txQElement = m_txQueue.front ();
      m_txPkt = txQElement->txQPkt;
      m_setMacState = Simulator::ScheduleNow (&LrWpanMac::SetLrWpanMacState, this, MAC_CSMA);
    }
}

void
LrWpanMac::SetCsmaCa (Ptr<LrWpanCsmaCa> csmaCa)
{
  m_csmaCa = csmaCa;
}

void
LrWpanMac::SetPhy (Ptr<LrWpanPhy> phy)
{
  m_phy = phy;
}

Ptr<LrWpanPhy>
LrWpanMac::GetPhy (void)
{
  return m_phy;
}

void
LrWpanMac::SetMcpsDataIndicationCallback (McpsDataIndicationCallback c)
{
  m_mcpsDataIndicationCallback = c;
}

void
LrWpanMac::SetMcpsDataConfirmCallback (McpsDataConfirmCallback c)
{
  m_mcpsDataConfirmCallback = c;
}

void
LrWpanMac::SetRfMacEnergyIndicationCallback (RfMacEnergyIndicationCallback c)
{
  m_rfMacEnergyIndicationCallback = c;
}

void
LrWpanMac::PdDataIndication (uint32_t psduLength, Ptr<Packet> p, uint8_t lqi)
{
  NS_LOG_DEBUG ("state: "<<m_lrWpanMacState);
  NS_ASSERT (m_lrWpanMacState == MAC_IDLE
              || m_lrWpanMacState == MAC_ACK_PENDING
              || m_lrWpanMacState == MAC_CSMA
              || m_lrWpanMacState == MAC_CFE_PENDING
              || m_lrWpanMacState == MAC_CFE_ACK_PENDING
              || m_lrWpanMacState == MAC_ENERGY_PENDING);

  NS_LOG_FUNCTION (this << psduLength << p << lqi);

  bool acceptFrame;

  // from sec 7.5.6.2 Reception and rejection, Std802.15.4-2006
  // level 1 filtering, test FCS field and reject if frame fails
  // level 2 filtering if promiscuous mode pass frame to higher layer otherwise perform level 3 filtering
  // level 3 filtering accept frame
  // if Frame type and version is not reserved, and
  // if there is a dstPanId then dstPanId=m_macPanId or broadcastPanI, and
  // if there is a shortDstAddr then shortDstAddr =shortMacAddr or broadcastAddr, and
  // if beacon frame then srcPanId = m_macPanId
  // if only srcAddr field in Data or Command frame,accept frame if srcPanId=m_macPanId

  Ptr<Packet> originalPkt = p->Copy (); // because we will strip headers

  m_promiscSnifferTrace (originalPkt);

  m_macPromiscRxTrace (originalPkt);
  // XXX no rejection tracing (to macRxDropTrace) being performed below

  LrWpanMacTrailer receivedMacTrailer;
  p->RemoveTrailer (receivedMacTrailer);
  if (Node::ChecksumEnabled ())
    {
      receivedMacTrailer.EnableFcs (true);
    }

  // level 1 filtering
  if (!receivedMacTrailer.CheckFcs (p))
    {
      m_macRxDropTrace (originalPkt);
    }
  else
    {
      LrWpanMacHeader receivedMacHdr;
      p->RemoveHeader (receivedMacHdr);

      McpsDataIndicationParams params;
      params.m_dsn = receivedMacHdr.GetSeqNum ();
      params.m_mpduLinkQuality = lqi;
      params.m_srcPanId = receivedMacHdr.GetSrcPanId ();
      params.m_srcAddrMode = receivedMacHdr.GetSrcAddrMode ();
      // TODO: Add field for EXT_ADDR source address.
      if (params.m_srcAddrMode == SHORT_ADDR)
        {
          params.m_srcAddr = receivedMacHdr.GetShortSrcAddr ();
        }
      params.m_dstPanId = receivedMacHdr.GetDstPanId ();
      params.m_dstAddrMode = receivedMacHdr.GetDstAddrMode ();
      // TODO: Add field for EXT_ADDR destination address.
      if (params.m_dstAddrMode == SHORT_ADDR)
        {
          params.m_dstAddr = receivedMacHdr.GetShortDstAddr ();
        }

      NS_LOG_DEBUG ("Packet from " << params.m_srcAddr << " to " << params.m_dstAddr);

      if (receivedMacHdr.GetType () == LrWpanMacHeader::LRWPAN_MAC_DATA)
        {
          Ptr<UniformRandomVariable> perValue = CreateObject<UniformRandomVariable> ();
          perValue->SetAttribute ("Min", DoubleValue (0.0));
          perValue->SetAttribute ("Max", DoubleValue (1.0));
          double per = perValue->GetValue ();
          NS_LOG_DEBUG ("per value: "<<per);
          if (per < 0.75)
            {
              m_macRxDropTrace (originalPkt);
              NS_LOG_DEBUG ("drop the packet");
              return;
            }
        }

      if (m_macPromiscuousMode)
        {
          //level 2 filtering
          if (!m_mcpsDataIndicationCallback.IsNull ())
            {
              NS_LOG_DEBUG ("promiscuous mode, forwarding up");
              m_mcpsDataIndicationCallback (params, p);
            }
          else
            {
              NS_LOG_ERROR (this << " Data Indication Callback not initialised");
            }
        }
      else
        {
          //level 3 frame filtering
          acceptFrame = (receivedMacHdr.GetType () != LrWpanMacHeader::LRWPAN_MAC_RESERVED);

          if (acceptFrame)
            {
              acceptFrame = (receivedMacHdr.GetFrameVer () <= 1);
            }

          if (acceptFrame
              && (receivedMacHdr.GetDstAddrMode () > 1))
            {
              acceptFrame = receivedMacHdr.GetDstPanId () == m_macPanId
                || receivedMacHdr.GetDstPanId () == 0xffff;
            }

          if (acceptFrame
              && (receivedMacHdr.GetDstAddrMode () == 2))
            {
              acceptFrame = receivedMacHdr.GetShortDstAddr () == m_shortAddress
                || receivedMacHdr.GetShortDstAddr () == Mac16Address ("ff:ff");        // check for broadcast addrs
            }

          if (acceptFrame
              && (receivedMacHdr.GetDstAddrMode () == 3))
            {
              acceptFrame = (receivedMacHdr.GetExtDstAddr () == m_selfExt);
            }

          if (acceptFrame
              && (receivedMacHdr.GetType () == LrWpanMacHeader::LRWPAN_MAC_BEACON))
            {
              if (m_macPanId == 0xffff)
                {
                  // TODO: Accept only if the frame version field is valid
                  acceptFrame = true;
                }
              else
                {
                  acceptFrame = receivedMacHdr.GetSrcPanId () == m_macPanId;
                }
            }

          if (acceptFrame
              && ((receivedMacHdr.GetType () == LrWpanMacHeader::LRWPAN_MAC_DATA)
                  || (receivedMacHdr.GetType () == LrWpanMacHeader::LRWPAN_MAC_COMMAND))
              && (receivedMacHdr.GetSrcAddrMode () > 1))
            {
              acceptFrame = receivedMacHdr.GetSrcPanId () == m_macPanId; // \todo need to check if PAN coord
            }

          if (acceptFrame)
            {
              // NS_LOG_DEBUG ("acceptFrame is true "<<m_lrWpanMacState << " hdr type "<< receivedMacHdr.GetType ());
              m_macRxTrace (originalPkt);
              if (receivedMacHdr.IsRfMac ())
                {
                  NS_LOG_DEBUG ("This packet is rf-mac type");
                  if (m_lrWpanMacState == MAC_CSMA)
                    {
                      m_csmaCa->StopTimer ();
                    }

                  RfMacTypeTag typeTag;
                  originalPkt->PeekPacketTag (typeTag);
                  if (IsEdt ())
                    {
                      NS_LOG_DEBUG ("A edt node received. type : "<<typeTag.Get ());
                      if (typeTag.IsRfe ())
                        {
                          m_setMacState.Cancel ();
                          ChangeMacState (MAC_IDLE);
                          Time delay;

                          m_rfeSrcAddress = receivedMacHdr.GetShortSrcAddr ();
                          m_rfeSrcPanId = receivedMacHdr.GetSrcPanId ();

                          RfMacGroupTag groupTag;
                          originalPkt->PeekPacketTag (groupTag);

                          m_groupNumber = groupTag.Get ();
                          if (m_groupNumber == 1)
                            {
                              delay = MicroSeconds (0.0);
                            }
                          else if (m_groupNumber == 2)
                            {
                              delay = MicroSeconds (10.0);
                            }

                          m_setMacState = Simulator::Schedule (delay, &LrWpanMac::SendCfeAfterRfe, this);
                        }
                      else if (typeTag.IsCfeAck () && m_lrWpanMacState == MAC_CFE_ACK_PENDING)
                        {
                          m_setMacState.Cancel ();
                          ChangeMacState (MAC_IDLE);

                          RfMacDurationTag chargingTime;
                          originalPkt->PeekPacketTag (chargingTime);

                          m_setMacState = Simulator::ScheduleNow (&LrWpanMac::SendEnergyPulse, this, chargingTime.Get ());
                        }
                      else // A edt doesn't need to receive except rfe, ack for cfe.
                        {
                          m_macRxDropTrace (originalPkt);
                        }
                    }
                  else if(IsSensor ())
                    {
                      NS_LOG_DEBUG ("A sensor node received. type : "<<typeTag.Get ());
                      //If a sensor node receive the rfe packet, the sensor are forced to freeze their backoff timers and get into charging mode.
                      if (typeTag.IsRfe ()) 
                        {
                          m_setMacState.Cancel ();
                          ChangeMacState (MAC_ENERGY_PENDING);
                        }
                      if (typeTag.IsCfe ())
                        {
                          m_cfeDstAddress = receivedMacHdr.GetShortDstAddr ();
                          m_cfeDstPanId = receivedMacHdr.GetDstPanId ();
                        }
                    }
                }
              // \todo: What should we do if we receive a frame while waiting for an ACK?
              //        Especially if this frame has the ACK request bit set, should we reply with an ACK, possibly missing the pending ACK?

              // If the received frame is a frame with the ACK request bit set, we immediately send back an ACK.
              // If we are currently waiting for a pending ACK, we assume the ACK was lost and trigger a retransmission after sending the ACK.
              if ((receivedMacHdr.IsData () || receivedMacHdr.IsCommand ()) && receivedMacHdr.IsAckReq ()
                  && !(receivedMacHdr.GetDstAddrMode () == SHORT_ADDR && receivedMacHdr.GetShortDstAddr () == "ff:ff"))
                {
                  // If this is a data or mac command frame, which is not a broadcast,
                  // with ack req set, generate and send an ack frame.
                  // If there is a CSMA medium access in progress we cancel the medium access
                  // for sending the ACK frame. A new transmission attempt will be started
                  // after the ACK was send.
                  if (m_lrWpanMacState == MAC_ACK_PENDING)
                    {
                      m_ackWaitTimeout.Cancel ();
                      PrepareRetransmission ();
                    }
                  else if (m_lrWpanMacState == MAC_CSMA)
                    {
                      // \todo: If we receive a packet while doing CSMA/CA, should  we drop the packet because of channel busy,
                      //        or should we restart CSMA/CA for the packet after sending the ACK?
                      // Currently we simply restart CSMA/CA after sending the ACK.
                      // m_csmaCa->Cancel ();
                      m_csmaCa->StopTimer ();
                    }
                  // Cancel any pending MAC state change, ACKs have higher priority.
                  m_setMacState.Cancel ();
                  ChangeMacState (MAC_IDLE);
                  m_setMacState = Simulator::ScheduleNow (&LrWpanMac::SendAck, this, receivedMacHdr.GetSeqNum ());
                }

              if (receivedMacHdr.IsData () && !m_mcpsDataIndicationCallback.IsNull ())
                {
                  // If it is a data frame, push it up the stack.
                  NS_LOG_DEBUG ("PdDataIndication():  Packet is for me; forwarding up");
                  m_mcpsDataIndicationCallback (params, p);
                }
              else if (receivedMacHdr.IsAcknowledgment () && m_txPkt && m_lrWpanMacState == MAC_ACK_PENDING)
                {
                  NS_LOG_DEBUG ("PdDataIndication():  ACK");
                  LrWpanMacHeader macHdr;
                  m_txPkt->PeekHeader (macHdr);
                  if (receivedMacHdr.GetSeqNum () == macHdr.GetSeqNum ())
                    {
                      m_macTxOkTrace (m_txPkt);
                      // If it is an ACK with the expected sequence number, finish the transmission
                      // and notify the upper layer.
                      m_ackWaitTimeout.Cancel ();
                      if (!m_mcpsDataConfirmCallback.IsNull ())
                        {
                          TxQueueElement *txQElement = m_txQueue.front ();
                          McpsDataConfirmParams confirmParams;
                          confirmParams.m_msduHandle = txQElement->txQMsduHandle;
                          confirmParams.m_status = IEEE_802_15_4_SUCCESS;
                          m_mcpsDataConfirmCallback (confirmParams);
                        }
                      RemoveFirstTxQElement ();
                      m_setMacState.Cancel ();
                      m_setMacState = Simulator::ScheduleNow (&LrWpanMac::SetLrWpanMacState, this, MAC_IDLE);
                    }
                  else
                    {
                      // If it is an ACK with an unexpected sequence number, mark the current transmission as failed and start a retransmit. (cf 7.5.6.4.3)
                      m_ackWaitTimeout.Cancel ();
                      if (!PrepareRetransmission ())
                        {
                          m_setMacState.Cancel ();
                          m_setMacState = Simulator::ScheduleNow (&LrWpanMac::SetLrWpanMacState, this, MAC_IDLE);
                        }
                      else
                        {
                          m_setMacState.Cancel ();
                          m_setMacState = Simulator::ScheduleNow (&LrWpanMac::SetLrWpanMacState, this, MAC_CSMA);
                        }
                    }
                }
            }
          else
            {
              m_macRxDropTrace (originalPkt);
            }
        }
    }
}

void
LrWpanMac::PdEnergyIndication (double energy, uint8_t slotNumber)
{
  if (IsSensor ())
  {
    NS_LOG_FUNCTION (this << energy << "slot " << static_cast<uint32_t> (slotNumber));
    if(slotNumber == 1)
      {
        m_receivedEnergyFromFirstSlot = energy; 
        return;
      }
    else if(slotNumber == 2)
      {
        m_receivedEnergyFromSecondSlot = energy;
        if (GetShortAddress () == m_cfeDstAddress)
          {
            m_setMacState.Cancel ();
            ChangeMacState (MAC_IDLE);
            m_setMacState = Simulator::ScheduleNow (&LrWpanMac::SendAckAfterCfe, this);
          }
        return;
      }
  }

  if (slotNumber == 0 && m_lrWpanMacState == MAC_ENERGY_PENDING)
  {
    // check whether this device send the rfe packet or not.
    if (GetShortAddress () == m_cfeDstAddress)
      {

      }
    else
      {

      }

    if (!m_rfMacEnergyIndicationCallback.IsNull ())
      {
        m_rfMacEnergyIndicationCallback (energy);
      }
    m_txPkt = 0;
    m_setMacState.Cancel ();
    m_setMacState = Simulator::ScheduleNow (&LrWpanMac::SetLrWpanMacState, this, MAC_IDLE);
  }
}

void
LrWpanMac::SendAck (uint8_t seqno)
{
  NS_LOG_FUNCTION (this << static_cast<uint32_t> (seqno));

  NS_ASSERT (m_lrWpanMacState == MAC_IDLE);

  // Generate a corresponding ACK Frame.
  LrWpanMacHeader macHdr (LrWpanMacHeader::LRWPAN_MAC_ACKNOWLEDGMENT, seqno);
  LrWpanMacTrailer macTrailer;
  Ptr<Packet> ackPacket = Create<Packet> (0);
  ackPacket->AddHeader (macHdr);
  // Calculate FCS if the global attribute ChecksumEnable is set.
  if (Node::ChecksumEnabled ())
    {
      macTrailer.EnableFcs (true);
      macTrailer.SetFcs (ackPacket);
    }
  ackPacket->AddTrailer (macTrailer);

  // Enqueue the ACK packet for further processing
  // when the transmitter is activated.
  m_txPkt = ackPacket;

  // Switch transceiver to TX mode. Proceed sending the Ack on confirm.
  ChangeMacState (MAC_SENDING);
  m_phy->PlmeSetTRXStateRequest (IEEE_802_15_4_PHY_TX_ON);
}

void
LrWpanMac::SendRfeForEnergy (void)
{
  NS_LOG_FUNCTION (this);
	
  Ptr<Packet> ackPacket = Create<Packet> (0);

  RfMacTypeTag typeTag;
  typeTag.Set (RfMacTypeTag::RF_MAC_RFE);

  ackPacket->AddPacketTag (typeTag);

  LrWpanMacHeader macHdr (LrWpanMacHeader::LRWPAN_MAC_RF_MAC, 0);
  macHdr.SetSrcAddrMode (SHORT_ADDR);
  macHdr.SetSrcAddrFields (GetPanId (), GetShortAddress ());
  macHdr.SetDstAddrMode (SHORT_ADDR);
  macHdr.SetDstAddrFields (0, Mac16Address("ff:ff"));

  ackPacket->AddHeader (macHdr);

  LrWpanMacTrailer macTrailer;
  
  if (Node::ChecksumEnabled ())
    {
      macTrailer.EnableFcs (true);
      macTrailer.SetFcs (ackPacket);
    }
  ackPacket->AddTrailer (macTrailer);

  m_txPkt = ackPacket;

  Simulator::ScheduleNow (&LrWpanMac::SetLrWpanMacState, this, MAC_CSMA);
}


void
LrWpanMac::SendCfeAfterRfe (void)
{
  NS_LOG_FUNCTION (this);

  NS_ASSERT (m_lrWpanMacState == MAC_IDLE);

  Ptr<Packet> ackPacket = Create<Packet> (0);

  RfMacTypeTag typeTag;
  typeTag.Set (RfMacTypeTag::RF_MAC_CFE);

  RfMacDurationTag durationTag;
  durationTag.Set (MicroSeconds (10.0));

  ackPacket->AddPacketTag (typeTag);
  ackPacket->AddPacketTag (durationTag);

  LrWpanMacHeader macHdr (LrWpanMacHeader::LRWPAN_MAC_RF_MAC, 0);
  macHdr.SetSrcAddrMode (SHORT_ADDR);
  macHdr.SetSrcAddrFields (GetPanId (), GetShortAddress ());
  macHdr.SetDstAddrMode (SHORT_ADDR);
  macHdr.SetDstAddrFields (m_rfeSrcPanId, m_rfeSrcAddress);

  ackPacket->AddHeader (macHdr);

  LrWpanMacTrailer macTrailer;
  // Calculate FCS if the global attribute ChecksumEnable is set.
  if (Node::ChecksumEnabled ())
    {
      macTrailer.EnableFcs (true);
      macTrailer.SetFcs (ackPacket);
    }
  ackPacket->AddTrailer (macTrailer);

  m_txPkt = ackPacket;

  SendNow ();
}

void
LrWpanMac::SendAckAfterCfe (void)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_lrWpanMacState == MAC_IDLE);

  Ptr<Packet> ackPacket = Create<Packet> (0);

  //Frequency Optimization and Calculate the charging time.
  RfMacTypeTag typeTag;
  typeTag.Set (RfMacTypeTag::RF_MAC_CFE_ACK);

  //need to calculate charging time T
  double requiredEnergy = 0.5*36*(m_maxThresholdVoltage*m_maxThresholdVoltage - m_minThresholdVoltage*m_minThresholdVoltage);
  double time = requiredEnergy / (m_receivedEnergyFromFirstSlot + m_receivedEnergyFromSecondSlot);
  NS_LOG_DEBUG ("max v: "<<m_maxThresholdVoltage<< " min v: "<<m_minThresholdVoltage<< " required energy: "<<requiredEnergy<<" charging time: "<<time);
  Time chargingTime = Seconds (time);

  RfMacDurationTag durationTag;
  durationTag.Set (chargingTime);

  ackPacket->AddPacketTag (typeTag);
  ackPacket->AddPacketTag (durationTag);

  // Generate a corresponding ACK Frame.
  LrWpanMacHeader macHdr (LrWpanMacHeader::LRWPAN_MAC_RF_MAC, 0);
  macHdr.SetSrcAddrMode (SHORT_ADDR);
  macHdr.SetSrcAddrFields (GetPanId (), GetShortAddress ());
  macHdr.SetDstAddrMode (SHORT_ADDR);
  macHdr.SetDstAddrFields (0, Mac16Address("ff:ff"));

  ackPacket->AddHeader (macHdr);

  LrWpanMacTrailer macTrailer;
  // Calculate FCS if the global attribute ChecksumEnable is set.
  if (Node::ChecksumEnabled ())
    {
      macTrailer.EnableFcs (true);
      macTrailer.SetFcs (ackPacket);
    }

  ackPacket->AddTrailer (macTrailer);

  m_txPkt = ackPacket;

  SendNow ();
}

void
LrWpanMac::SendEnergyPulse (Time chargingTime)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT (m_lrWpanMacState == MAC_IDLE);

  Ptr<Packet> energyPulse = Create<Packet> (0);

  RfMacTypeTag typeTag;
  typeTag.Set (RfMacTypeTag::RF_MAC_ENERGY);

  RfMacDurationTag durationTag;
  durationTag.Set (chargingTime);

  energyPulse->AddPacketTag (typeTag);
  energyPulse->AddPacketTag (durationTag);

  LrWpanMacHeader macHdr (LrWpanMacHeader::LRWPAN_MAC_RF_MAC, 0);
  macHdr.SetSrcAddrMode (SHORT_ADDR);
  macHdr.SetSrcAddrFields (GetPanId (), GetShortAddress ());
  macHdr.SetDstAddrMode (SHORT_ADDR);
  macHdr.SetDstAddrFields (0, Mac16Address("ff:ff"));

  energyPulse->AddHeader (macHdr);

  LrWpanMacTrailer macTrailer;
  // Calculate FCS if the global attribute ChecksumEnable is set.
  if (Node::ChecksumEnabled ())
    {
      macTrailer.EnableFcs (true);
      macTrailer.SetFcs (energyPulse);
    }

  energyPulse->AddTrailer (macTrailer);

  m_txPkt = energyPulse;

  SendNow ();
}

void
LrWpanMac::SendNow (void)
{
  NS_LOG_FUNCTION (this);
  // m_setMacState = Simulator::ScheduleNow (&LrWpanMac::SetLrWpanMacState, this, MAC_CSMA);
  ChangeMacState (MAC_SENDING);
  m_phy->PlmeSetTRXStateRequest (IEEE_802_15_4_PHY_TX_ON);
}

void
LrWpanMac::RemoveFirstTxQElement ()
{
  TxQueueElement *txQElement = m_txQueue.front ();
  Ptr<const Packet> p = txQElement->txQPkt;
  m_numCsmacaRetry += m_csmaCa->GetNB () + 1;

  Ptr<Packet> pkt = p->Copy ();
  LrWpanMacHeader hdr;
  pkt->RemoveHeader (hdr);
  if (hdr.GetShortDstAddr () != Mac16Address ("ff:ff"))
    {
      m_sentPktTrace (p, m_retransmission + 1, m_numCsmacaRetry);
    }

  txQElement->txQPkt = 0;
  delete txQElement;
  m_txQueue.pop_front ();
  m_txPkt = 0;
  m_retransmission = 0;
  m_numCsmacaRetry = 0;
  m_macTxDequeueTrace (p);
}

void
LrWpanMac::AckWaitTimeout (void)
{
  NS_LOG_FUNCTION (this);

  // TODO: If we are a PAN coordinator and this was an indirect transmission,
  //       we will not initiate a retransmission. Instead we wait for the data
  //       being extracted after a new data request command.
  if (!PrepareRetransmission ())
    {
      SetLrWpanMacState (MAC_IDLE);
    }
  else
    {
      SetLrWpanMacState (MAC_CSMA);
    }
}

bool
LrWpanMac::PrepareRetransmission (void)
{
  NS_LOG_FUNCTION (this);

  if (m_retransmission >= m_macMaxFrameRetries)
    {
      // Maximum number of retransmissions has been reached.
      // remove the copy of the packet that was just sent
      TxQueueElement *txQElement = m_txQueue.front ();
      m_macTxDropTrace (txQElement->txQPkt);
      if (!m_mcpsDataConfirmCallback.IsNull ())
        {
          McpsDataConfirmParams confirmParams;
          confirmParams.m_msduHandle = txQElement->txQMsduHandle;
          confirmParams.m_status = IEEE_802_15_4_NO_ACK;
          m_mcpsDataConfirmCallback (confirmParams);
        }
      RemoveFirstTxQElement ();
      return false;
    }
  else
    {
      m_retransmission++;
      m_numCsmacaRetry += m_csmaCa->GetNB () + 1;
      // Start next CCA process for this packet.
      return true;
    }
}

void
LrWpanMac::PdDataConfirm (LrWpanPhyEnumeration status)
{
  NS_ASSERT (m_lrWpanMacState == MAC_SENDING);

  NS_LOG_FUNCTION (this << status << m_txQueue.size ());

  LrWpanMacHeader macHdr;
  m_txPkt->PeekHeader (macHdr);
  if (status == IEEE_802_15_4_PHY_SUCCESS)
    {
      if (!macHdr.IsAcknowledgment ())
        {
          // We have just send a regular data packet, check if we have to wait
          // for an ACK.
          if (macHdr.IsAckReq ())
            {
              // wait for the ack or the next retransmission timeout
              // start retransmission timer
              // if (macHdr.IsRfe ())
              //   {
              //     // Time waitTime = MicroSeconds (GetMacAckWaitDuration () * 1000 * 1000 / m_phy->GetDataOrSymbolRate (false));
              //     // NS_ASSERT (m_ackWaitTimeout.IsExpired ());
              //     // m_ackWaitTimeout = Simulator::Schedule (waitTime, &LrWpanMac::AckWaitTimeout, this);
              //     m_setMacState.Cancel ();
              //     m_setMacState = Simulator::ScheduleNow (&LrWpanMac::SetLrWpanMacState, this, MAC_CFE_PENDING);
              //     return;
              //   }
              // else if (macHdr.IsCfe ())
              //   {
              //     m_setMacState.Cancel ();
              //     m_setMacState = Simulator::ScheduleNow (&LrWpanMac::SetLrWpanMacState, this, MAC_CFE_ACK_PENDING); 
              //     return;
              //   }
              // else
              //   {
                  Time waitTime = MicroSeconds (GetMacAckWaitDuration () * 1000 * 1000 / m_phy->GetDataOrSymbolRate (false));
                  NS_ASSERT (m_ackWaitTimeout.IsExpired ());
                  m_ackWaitTimeout = Simulator::Schedule (waitTime, &LrWpanMac::AckWaitTimeout, this);
                  m_setMacState.Cancel ();
                  m_setMacState = Simulator::ScheduleNow (&LrWpanMac::SetLrWpanMacState, this, MAC_ACK_PENDING);
                  return;
                // }
            }
          else if (macHdr.IsRfMac ())
            {
              RfMacTypeTag typeTag;
              m_txPkt->PeekPacketTag (typeTag);

              m_setMacState.Cancel ();

              if (typeTag.IsRfe ())
                {
                  m_setMacState = Simulator::ScheduleNow (&LrWpanMac::SetLrWpanMacState, this, MAC_CFE_PENDING);
                }
              else if (typeTag.IsCfe ())
                {
                  m_setMacState = Simulator::ScheduleNow (&LrWpanMac::SetLrWpanMacState, this, MAC_CFE_ACK_PENDING);
                }
              else if (typeTag.IsCfeAck ())
                {
                  m_setMacState = Simulator::ScheduleNow (&LrWpanMac::SetLrWpanMacState, this, MAC_ENERGY_PENDING);
                }
              else if (typeTag.IsEnergy ())
                {
                  m_setMacState = Simulator::ScheduleNow (&LrWpanMac::SetLrWpanMacState, this, MAC_IDLE);
                }
                return;
            }
          else
            {
              m_macTxOkTrace (m_txPkt);
              // remove the copy of the packet that was just sent
              if (!m_mcpsDataConfirmCallback.IsNull ())
                {
                  McpsDataConfirmParams confirmParams;
                  NS_ASSERT_MSG (m_txQueue.size () > 0, "TxQsize = 0");
                  TxQueueElement *txQElement = m_txQueue.front ();
                  confirmParams.m_msduHandle = txQElement->txQMsduHandle;
                  confirmParams.m_status = IEEE_802_15_4_SUCCESS;
                  m_mcpsDataConfirmCallback (confirmParams);
                }
              RemoveFirstTxQElement ();
            }
        }
      else
        {
          // We have send an ACK. Clear the packet buffer.
          m_txPkt = 0;
        }
    }
  else if (status == IEEE_802_15_4_PHY_UNSPECIFIED)
    {

      if (!macHdr.IsAcknowledgment ())
        {
          NS_ASSERT_MSG (m_txQueue.size () > 0, "TxQsize = 0");
          TxQueueElement *txQElement = m_txQueue.front ();
          m_macTxDropTrace (txQElement->txQPkt);
          if (!m_mcpsDataConfirmCallback.IsNull ())
            {
              McpsDataConfirmParams confirmParams;
              confirmParams.m_msduHandle = txQElement->txQMsduHandle;
              confirmParams.m_status = IEEE_802_15_4_FRAME_TOO_LONG;
              m_mcpsDataConfirmCallback (confirmParams);
            }
          RemoveFirstTxQElement ();
        }
      else
        {
          NS_LOG_ERROR ("Unable to send ACK");
        }
    }
  else
    {
      // Something went really wrong. The PHY is not in the correct state for
      // data transmission.
      NS_FATAL_ERROR ("Transmission attempt failed with PHY status " << status);
    }

  m_setMacState.Cancel ();
  m_setMacState = Simulator::ScheduleNow (&LrWpanMac::SetLrWpanMacState, this, MAC_IDLE);
}

void
LrWpanMac::PlmeCcaConfirm (LrWpanPhyEnumeration status)
{
  NS_LOG_FUNCTION (this << status);
  // Direct this call through the csmaCa object
  m_csmaCa->PlmeCcaConfirm (status);
}

void
LrWpanMac::PlmeEdConfirm (LrWpanPhyEnumeration status, uint8_t energyLevel)
{
  NS_LOG_FUNCTION (this << status << energyLevel);
}

void
LrWpanMac::PlmeGetAttributeConfirm (LrWpanPhyEnumeration status,
                                    LrWpanPibAttributeIdentifier id,
                                    LrWpanPhyPibAttributes* attribute)
{
  NS_LOG_FUNCTION (this << status << id << attribute);
}

void
LrWpanMac::PlmeSetTRXStateConfirm (LrWpanPhyEnumeration status)
{
  NS_LOG_FUNCTION (this << status);

  if (m_lrWpanMacState == MAC_SENDING && (status == IEEE_802_15_4_PHY_TX_ON || status == IEEE_802_15_4_PHY_SUCCESS))
    {
      NS_ASSERT (m_txPkt);

      // Start sending if we are in state SENDING and the PHY transmitter was enabled.
      m_promiscSnifferTrace (m_txPkt);
      m_snifferTrace (m_txPkt);
      m_macTxTrace (m_txPkt);
      m_phy->PdDataRequest (m_txPkt->GetSize (), m_txPkt);
    }
  else if (m_lrWpanMacState == MAC_CSMA && (status == IEEE_802_15_4_PHY_RX_ON || status == IEEE_802_15_4_PHY_SUCCESS))
    {
      // Start the CSMA algorithm as soon as the receiver is enabled.
      m_csmaCa->Start ();
    }
  else if (m_lrWpanMacState == MAC_IDLE)
    {
      NS_ASSERT (status == IEEE_802_15_4_PHY_RX_ON || status == IEEE_802_15_4_PHY_SUCCESS || status == IEEE_802_15_4_PHY_TRX_OFF);
      // Do nothing special when going idle.
    }
  else if (m_lrWpanMacState == MAC_ACK_PENDING || m_lrWpanMacState == MAC_CFE_PENDING || m_lrWpanMacState == MAC_CFE_ACK_PENDING || m_lrWpanMacState == MAC_ENERGY_PENDING)
    {
      NS_ASSERT (status == IEEE_802_15_4_PHY_RX_ON || status == IEEE_802_15_4_PHY_SUCCESS);
    }
  else
    {
      // TODO: What to do when we receive an error?
      // If we want to transmit a packet, but switching the transceiver on results
      // in an error, we have to recover somehow (and start sending again).
      NS_FATAL_ERROR ("Error changing transceiver state");
    }
}

void
LrWpanMac::PlmeSetAttributeConfirm (LrWpanPhyEnumeration status,
                                    LrWpanPibAttributeIdentifier id)
{
  NS_LOG_FUNCTION (this << status << id);
}

void
LrWpanMac::SetLrWpanMacState (LrWpanMacState macState)
{
  NS_LOG_FUNCTION (this << "mac state = " << macState);

  McpsDataConfirmParams confirmParams;

  if (macState == MAC_IDLE)
    {
      ChangeMacState (MAC_IDLE);

      if (m_macRxOnWhenIdle)
        {
          m_phy->PlmeSetTRXStateRequest (IEEE_802_15_4_PHY_RX_ON);
        }
      else
        {
          m_phy->PlmeSetTRXStateRequest (IEEE_802_15_4_PHY_TRX_OFF);
        }

      CheckQueue ();
    }
  else if (macState == MAC_ACK_PENDING)
    {
      ChangeMacState (MAC_ACK_PENDING);
      m_phy->PlmeSetTRXStateRequest (IEEE_802_15_4_PHY_RX_ON);
    }
  else if (macState == MAC_CFE_PENDING)
    {
      ChangeMacState (MAC_CFE_PENDING);
      m_phy->PlmeSetTRXStateRequest (IEEE_802_15_4_PHY_RX_ON);
    }
  else if (macState == MAC_CFE_ACK_PENDING)
    {
      ChangeMacState (MAC_CFE_ACK_PENDING);
      m_phy->PlmeSetTRXStateRequest (IEEE_802_15_4_PHY_RX_ON);  
    }
  else if (macState == MAC_ENERGY_PENDING)
    {
      ChangeMacState (MAC_ENERGY_PENDING);
      m_phy->PlmeSetTRXStateRequest (IEEE_802_15_4_PHY_RX_ON); 
    }
  else if (macState == MAC_CSMA)
    {
      NS_ASSERT (m_lrWpanMacState == MAC_IDLE
                  || m_lrWpanMacState == MAC_ACK_PENDING
                  || m_lrWpanMacState == MAC_CFE_PENDING
                  || m_lrWpanMacState == MAC_CFE_ACK_PENDING
                  || m_lrWpanMacState == MAC_ENERGY_PENDING);

      ChangeMacState (MAC_CSMA);
      m_phy->PlmeSetTRXStateRequest (IEEE_802_15_4_PHY_RX_ON);
    }
  else if (m_lrWpanMacState == MAC_CSMA && macState == CHANNEL_IDLE)
    {
      // Channel is idle, set transmitter to TX_ON
      ChangeMacState (MAC_SENDING);
      m_phy->PlmeSetTRXStateRequest (IEEE_802_15_4_PHY_TX_ON);
    }
  else if (m_lrWpanMacState == MAC_CSMA && macState == CHANNEL_ACCESS_FAILURE)
    {
      NS_ASSERT (m_txPkt);

      // cannot find a clear channel, drop the current packet.
      NS_LOG_DEBUG ( this << " cannot find clear channel");
      confirmParams.m_msduHandle = m_txQueue.front ()->txQMsduHandle;
      confirmParams.m_status = IEEE_802_15_4_CHANNEL_ACCESS_FAILURE;
      m_macTxDropTrace (m_txPkt);
      if (!m_mcpsDataConfirmCallback.IsNull ())
        {
          m_mcpsDataConfirmCallback (confirmParams);
        }
      // remove the copy of the packet that was just sent
      RemoveFirstTxQElement ();

      ChangeMacState (MAC_IDLE);
    }
}

LrWpanAssociationStatus
LrWpanMac::GetAssociationStatus (void) const
{
  return m_associationStatus;
}

void
LrWpanMac::SetAssociationStatus (LrWpanAssociationStatus status)
{
  m_associationStatus = status;
}

uint16_t
LrWpanMac::GetPanId (void) const
{
  return m_macPanId;
}

void
LrWpanMac::SetPanId (uint16_t panId)
{
  m_macPanId = panId;
}

void
LrWpanMac::ChangeMacState (LrWpanMacState newState)
{
  NS_LOG_LOGIC (this << " change lrwpan mac state from "
                     << m_lrWpanMacState << " to "
                     << newState);
  m_macStateLogger (m_lrWpanMacState, newState);
  m_lrWpanMacState = newState;
}

uint64_t
LrWpanMac::GetMacAckWaitDuration (void) const
{
  return m_csmaCa->GetUnitBackoffPeriod () + m_phy->aTurnaroundTime + m_phy->GetPhySHRDuration ()
         + ceil (6 * m_phy->GetPhySymbolsPerOctet ());
}

uint64_t
LrWpanMac::GetMacCfeWaitDuration (void) const
{
  return m_csmaCa->GetUnitBackoffPeriod () + m_phy->aTurnaroundTime + m_phy->GetPhySHRDuration ()
         + ceil (6 * m_phy->GetPhySymbolsPerOctet ());  
}

uint8_t
LrWpanMac::GetMacMaxFrameRetries (void) const
{
  return m_macMaxFrameRetries;
}

void
LrWpanMac::SetMacMaxFrameRetries (uint8_t retries)
{
  m_macMaxFrameRetries = retries;
}

Time
LrWpanMac::GetSlotTimeOfEnergy (void) const
{
  return m_slotTimeOfEnergy;
}

Time
LrWpanMac::GetSlotTimeOfData (void) const
{
  return m_slotTimeOfData;
}

Time
LrWpanMac::GetDifsOfEnergy (void) const
{
  return m_difsOfEnergy;
}

Time
LrWpanMac::GetDifsOfData (void) const
{
  return m_difsOfData;
}

Time
LrWpanMac::GetSifsOfEnergy (void) const
{
  return m_sifsOfEnergy;
}

Time
LrWpanMac::GetSifsOfData (void) const
{
  return m_sifsOfData;
}

void
LrWpanMac::SetDeviceType (LrWpanMacDeviceType type)
{
  m_deviceType = type;
}

bool
LrWpanMac::IsSensor (void)
{
  return (m_deviceType == MAC_FOR_SENSOR);
}

bool
LrWpanMac::IsEdt (void)
{
  return (m_deviceType == MAC_FOR_EDT);
}

} // namespace ns3
