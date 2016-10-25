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
#include <ns3/packet.h>

#include "ns3/mobility-module.h"

#include <iostream>

using namespace ns3;

// static void DataIndication (McpsDataIndicationParams params, Ptr<Packet> p)
// {
//   // NS_LOG_UNCOND ("Received packet of size " << p->GetSize ());
//   // p->Print (std::cout);
// }

// static void DataConfirm (McpsDataConfirmParams params)
// {
//   // NS_LOG_UNCOND ("LrWpanMcpsDataConfirmStatus = " << params.m_status);
// }

static void StateChangeNotification (std::string context, Time now, LrWpanPhyEnumeration oldState, LrWpanPhyEnumeration newState)
{
  NS_LOG_UNCOND (context << " state change at " << now.GetSeconds ()
                         << " from " << LrWpanHelper::LrWpanPhyEnumerationPrinter (oldState)
                         << " to " << LrWpanHelper::LrWpanPhyEnumerationPrinter (newState));
}

int main (int argc, char *argv[])
{
  bool verbose = false;
  uint8_t nSensorNode = 1;
  uint8_t nEnergyNode = 3;

  CommandLine cmd;

  cmd.AddValue ("verbose", "turn on all log components", verbose);
  // cmd.AddValue ("nEnergyNode", "the number of energy nodes", nEnergyNode);
  // cmd.AddValue ("nSensorNode", "the number of sensor nodes", nSensorNode);

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

  Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel> ();
  Ptr<LogDistancePropagationLossModel> propModel = CreateObject<LogDistancePropagationLossModel> ();
  Ptr<ConstantSpeedPropagationDelayModel> delayModel = CreateObject<ConstantSpeedPropagationDelayModel> ();
  channel->AddPropagationLossModel (propModel);
  channel->SetPropagationDelayModel (delayModel);

  NodeContainer nodes;
  nodes.Create(nSensorNode + nEnergyNode);

  MobilityHelper mobility;
  mobility.SetPositionAllocator("ns3::RandomDiscPositionAllocator",
                                "X", StringValue ("100.0"),
                                "Y", StringValue ("100.0"),
                                "Rho", StringValue ("ns3::UniformRandomVariable[Min=0|Max=2.5]"));
  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.Install (nodes);

  Mac16Address macAddress = Mac16Address ();

  for(int i=0; i<nSensorNode; ++i)
    {
      Ptr<LrWpanSensorNetDevice> dev = CreateObject<LrWpanSensorNetDevice> ();
      dev->SetAddress (macAddress.Allocate());
      dev->SetChannel (channel);
      nodes.Get (i)->AddDevice (dev);
      dev->GetPhy ()->TraceConnect ("TrxState", std::string ("phy"+std::to_string (i)), MakeCallback (&StateChangeNotification));
    }

  for(int i = nSensorNode; i<nSensorNode + nEnergyNode; ++i)
    {
      Ptr<LrWpanEdtNetDevice> dev = CreateObject<LrWpanEdtNetDevice> ();
      dev->SetAddress (macAddress.Allocate());
      dev->SetChannel (channel);
      nodes.Get (i)->AddDevice (dev);
      dev->GetPhy ()->TraceConnect ("TrxState", std::string ("phy"+std::to_string (i)), MakeCallback (&StateChangeNotification));
    }

  lrWpanHelper.EnablePcapAll (std::string ("rf-mac-energy-data"), true);
  AsciiTraceHelper ascii;
  Ptr<OutputStreamWrapper> stream = ascii.CreateFileStream ("rf-mac-energy-data.tr");
  lrWpanHelper.EnableAsciiAll (stream);

  Ptr<Packet> p0 = Create<Packet> (50);  // 50 bytes of dummy data
  McpsDataRequestParams params;
  params.m_srcAddrMode = SHORT_ADDR;
  params.m_dstAddrMode = SHORT_ADDR;
  params.m_dstPanId = 0;
  params.m_dstAddr = Mac16Address (std::string("00:01").c_str ());
  params.m_msduHandle = 0;
  params.m_txOptions = TX_OPTION_ACK;

  Ptr<LrWpanSensorNetDevice> dev ((nodes.Get (0)->GetDevice (0)->GetObject<LrWpanSensorNetDevice> ()));
  // Ptr<LrWpanSensorNetDevice> dev2 ((nodes.Get (1)->GetDevice (0)->GetObject<LrWpanSensorNetDevice> ()));
  Simulator::ScheduleWithContext (1, Seconds(1.0),
                                &LrWpanMac::SendRfeForEnergy,
                                 dev->GetMac ());
  // Simulator::ScheduleWithContext (2, Seconds(1.0),
  //                               &LrWpanMac::McpsDataRequest,
  //                               dev2->GetMac(), params, p0);
  Simulator::Stop (Seconds (10.0));
  Simulator::Run ();
  Simulator::Destroy ();
  return 0;
}
