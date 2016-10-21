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
 * Author:  Tom Henderson <thomas.r.henderson@boeing.com>
 */

/*
 * Try to send data end-to-end through a LrWpanMac <-> LrWpanPhy <->
 * SpectrumChannel <-> LrWpanPhy <-> LrWpanMac chain
 *
 * Trace Phy state changes, and Mac DataIndication and DataConfirm events
 * to stdout
 */
#include <ns3/log.h>
#include <ns3/core-module.h>
#include <ns3/lr-wpan-module.h>
#include <ns3/propagation-loss-model.h>
#include <ns3/propagation-delay-model.h>
#include <ns3/simulator.h>
#include <ns3/single-model-spectrum-channel.h>
#include <ns3/constant-position-mobility-model.h>
#include <ns3/packet.h>

#include <iostream>

using namespace ns3;

static void DataIndication (McpsDataIndicationParams params, Ptr<Packet> p)
{
  // NS_LOG_UNCOND ("Received packet of size " << p->GetSize ());
  // p->Print (std::cout);
}

static void DataConfirm (McpsDataConfirmParams params)
{
  // NS_LOG_UNCOND ("LrWpanMcpsDataConfirmStatus = " << params.m_status);
}

static void StateChangeNotification (std::string context, Time now, LrWpanPhyEnumeration oldState, LrWpanPhyEnumeration newState)
{
  NS_LOG_UNCOND (context << " state change at " << now.GetSeconds ()
                         << " from " << LrWpanHelper::LrWpanPhyEnumerationPrinter (oldState)
                         << " to " << LrWpanHelper::LrWpanPhyEnumerationPrinter (newState));
}

int main (int argc, char *argv[])
{
  bool verbose = false;
  uint8_t nEnergyNode = 1;
  uint8_t nSensorNode = 1;

  CommandLine cmd;

  cmd.AddValue ("verbose", "turn on all log components", verbose);
  cmd.AddValue ("nEnergyNode", "the number of energy nodes", nEnergyNode);
  cmd.AddValue ("nSensorNode", "the number of sensor nodes", nSensorNode);

  cmd.Parse (argc, argv);

  LrWpanHelper lrWpanHelper;
  if (verbose)
    {
      lrWpanHelper.EnableLogComponents ();
    }

  Packet::EnablePrinting ();
  Packet::EnableChecking ();
  // Enable calculation of FCS in the trailers. Only necessary when interacting with real devices or wireshark.
  // GlobalValue::Bind ("ChecksumEnabled", BooleanValue (true));

  // Create 2 nodes, and a NetDevice for each one
  Ptr<Node> n0 = CreateObject <Node> ();
  Ptr<Node> n1 = CreateObject <Node> ();
  Ptr<Node> n2 = CreateObject <Node> ();
  Ptr<Node> n3 = CreateObject <Node> ();

  Ptr<LrWpanSensorNetDevice> dev0 = CreateObject<LrWpanSensorNetDevice> ();
  Ptr<LrWpanSensorNetDevice> dev1 = CreateObject<LrWpanSensorNetDevice> ();
  Ptr<LrWpanEdtNetDevice> dev2 = CreateObject<LrWpanEdtNetDevice> ();
  Ptr<LrWpanEdtNetDevice> dev3 = CreateObject<LrWpanEdtNetDevice> ();

  dev0->SetAddress (Mac16Address ("00:01"));
  dev1->SetAddress (Mac16Address ("00:02"));
  dev2->SetAddress (Mac16Address ("00:03"));
  dev3->SetAddress (Mac16Address ("00:04"));


  Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel> ();
  Ptr<LogDistancePropagationLossModel> propModel = CreateObject<LogDistancePropagationLossModel> ();
  Ptr<ConstantSpeedPropagationDelayModel> delayModel = CreateObject<ConstantSpeedPropagationDelayModel> ();
  channel->AddPropagationLossModel (propModel);
  channel->SetPropagationDelayModel (delayModel);
  // Each device must be attached to the same channel

  dev0->SetChannel (channel);
  dev1->SetChannel (channel);
  dev2->SetChannel (channel);
  dev3->SetChannel (channel);


  // To complete configuration, a LrWpanNetDevice must be added to a node
  n0->AddDevice (dev0);
  n1->AddDevice (dev1);
  n2->AddDevice (dev2);
  n3->AddDevice (dev3);

  // Trace state changes in the phy
  dev0->GetPhy ()->TraceConnect ("TrxState", std::string ("phy0"), MakeCallback (&StateChangeNotification));
  dev1->GetPhy ()->TraceConnect ("TrxState", std::string ("phy1"), MakeCallback (&StateChangeNotification));
  dev2->GetPhy ()->TraceConnect ("TrxState", std::string ("phy2"), MakeCallback (&StateChangeNotification));
  dev3->GetPhy ()->TraceConnect ("TrxState", std::string ("phy3"), MakeCallback (&StateChangeNotification));

  Ptr<ConstantPositionMobilityModel> sender0Mobility = CreateObject<ConstantPositionMobilityModel> ();
  sender0Mobility->SetPosition (Vector (0,0,0));
  dev0->GetPhy ()->SetMobility (sender0Mobility);
  Ptr<ConstantPositionMobilityModel> sender1Mobility = CreateObject<ConstantPositionMobilityModel> ();
  // Configure position 10 m distance
  sender1Mobility->SetPosition (Vector (0,1.5,0));
  dev1->GetPhy ()->SetMobility (sender1Mobility);

  Ptr<ConstantPositionMobilityModel> sender2Mobility = CreateObject<ConstantPositionMobilityModel> ();
  sender2Mobility->SetPosition (Vector (0,-2,0));
  dev2->GetPhy ()->SetMobility (sender2Mobility);

  Ptr<ConstantPositionMobilityModel> sender3Mobility = CreateObject<ConstantPositionMobilityModel> ();
  sender3Mobility->SetPosition (Vector (10,5,0));
  dev3->GetPhy ()->SetMobility (sender3Mobility);

  McpsDataConfirmCallback cb0;
  cb0 = MakeCallback (&DataConfirm);
  dev0->GetMac ()->SetMcpsDataConfirmCallback (cb0);

  McpsDataIndicationCallback cb1;
  cb1 = MakeCallback (&DataIndication);
  dev0->GetMac ()->SetMcpsDataIndicationCallback (cb1);

  McpsDataConfirmCallback cb2;
  cb2 = MakeCallback (&DataConfirm);
  dev1->GetMac ()->SetMcpsDataConfirmCallback (cb2);

  McpsDataIndicationCallback cb3;
  cb3 = MakeCallback (&DataIndication);
  dev1->GetMac ()->SetMcpsDataIndicationCallback (cb3);

  McpsDataConfirmCallback cb4;
  cb4 = MakeCallback (&DataConfirm);
  dev2->GetMac ()->SetMcpsDataConfirmCallback (cb4);

  McpsDataIndicationCallback cb5;
  cb5 = MakeCallback (&DataIndication);
  dev2->GetMac ()->SetMcpsDataIndicationCallback (cb5);

  McpsDataConfirmCallback cb6;
  cb6 = MakeCallback (&DataConfirm);
  dev3->GetMac ()->SetMcpsDataConfirmCallback (cb6);

  McpsDataIndicationCallback cb7;
  cb7 = MakeCallback (&DataIndication);
  dev3->GetMac ()->SetMcpsDataIndicationCallback (cb7);


  // Tracing
  lrWpanHelper.EnablePcapAll (std::string ("lr-wpan-rf-mac-data"), true);
  AsciiTraceHelper ascii;
  Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream ("lr-wpan-rf-mac-data.tr");
  lrWpanHelper.EnableAsciiAll (stream);

  // The below should trigger two callbacks when end-to-end data is working
  // 1) DataConfirm callback is called
  // 2) DataIndication callback is called with value of 50
  Ptr<Packet> p0 = Create<Packet> (50);  // 50 bytes of dummy data
  McpsDataRequestParams params;
  params.m_srcAddrMode = SHORT_ADDR;
  params.m_dstAddrMode = SHORT_ADDR;
  params.m_dstPanId = 0;
  params.m_dstAddr = Mac16Address ("00:02");
  params.m_msduHandle = 0;
  params.m_txOptions = TX_OPTION_ACK;
//  dev0->GetMac ()->McpsDataRequest (params, p0);


  p0->Print (std::cout);
  std::cout << std::endl;

  // Simulator::ScheduleWithContext (1, Seconds (1.0),
  //                                 &LrWpanMac::McpsDataRequest,
  //                                 dev0->GetMac (), params, p0);

  // Simulator::ScheduleWithContext (2, Seconds(2.0),
  //                                 &LrWpanMac::SendRfeForEnergy,
  //                                 dev0->GetMac ());

  Simulator::ScheduleWithContext (1, Seconds(1.0),
                                  &LrWpanMac::SendRfeForEnergy,
                                  dev0->GetMac ());

  Simulator::ScheduleWithContext (2, Seconds (2.0),
                                  &LrWpanMac::McpsDataRequest,
                                  dev0->GetMac (), params, p0);
  Simulator::Stop (Seconds (5.0));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
