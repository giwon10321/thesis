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

  Ptr<SingleModelSpectrumChannel> channel = CreateObject<SingleModelSpectrumChannel> ();
  Ptr<LogDistancePropagationLossModel> propModel = CreateObject<LogDistancePropagationLossModel> ();
  Ptr<ConstantSpeedPropagationDelayModel> delayModel = CreateObject<ConstantSpeedPropagationDelayModel> ();
  channel->AddPropagationLossModel (propModel);
  channel->SetPropagationDelayModel (delayModel);

  NodeContainer energyNodes;
  energyNodes.Create (nEnergyNode);

  NodeContainer sensorNodes;
  sensorNodes.Create (nSensorNode);

  for(int i=0; i<nSensorNode; i++)
    {
      Ptr<LrWpanSensorNetDevice> dev = CreateObject<LrWpanSensorNetDevice> ();
      std::string addressString ("00:0"+std::to_string (i));
      dev->SetAddress (Mac16Address (addressString.c_str ()));
      dev->SetChannel (channel);
      sensorNodes.Get (i)->AddDevice (dev);
      dev->GetPhy ()->TraceConnect ("TrxState", std::string ("phy0"), MakeCallback (&StateChangeNotification));
      
    }

  Simulator::Run ();

  Simulator::Destroy ();
  return 0;
}
