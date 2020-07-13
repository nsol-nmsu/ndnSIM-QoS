/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

// ndn-simple.cpp

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdlib.h>


#include "../apps/ndn-subscriber-sync.hpp"
//#include "../apps/ndn-synchronizer.hpp"
#include "../apps/ndn-synchronizer-socket.hpp"


namespace ns3 {

/**
 * This scenario simulates a very simple network topology:
 *
 *
 *      +----------+     1Mbps      +--------+     1Mbps      +----------+
 *      | consumer | <------------> | router | <------------> | producer |
 *      +----------+         10ms   +--------+          10ms  +----------+
 *
 *
 * Consumer requests data from producer with frequency 10 interests per second
 * (interests contain constantly increasing sequence number).
 *
 * For every received interest, producer replies with a data packet, containing
 * 1024 bytes of virtual payload.
 *
 * To run scenario and see what is happening, use the following command:
 *
 *     NS_LOG=ndn.Consumer:ndn.Producer ./waf --run=ndn-simple
 */
void ReceivedInterestCallback( uint32_t, shared_ptr<const ndn::Interest> );
void ReceivedDataCallback( uint32_t, shared_ptr<const ndn::Data> );
 ns3::ndn::SyncSocket sync;//= new  ns3::ndn::Synchronizer();

int
main(int argc, char* argv[])
{
  // setting default parameters for PointToPoint links and channels
  Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("1Mbps"));
  Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("10ms"));
  Config::SetDefault("ns3::QueueBase::MaxSize", StringValue("20p"));

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.Parse(argc, argv);

  // Creating nodes
  NodeContainer nodes;
  nodes.Create(5);

  // Connecting nodes using two links
  PointToPointHelper p2p;
  p2p.Install(nodes.Get(0), nodes.Get(3));
  p2p.Install(nodes.Get(3), nodes.Get(4));
  p2p.Install(nodes.Get(1), nodes.Get(3));
  p2p.Install(nodes.Get(2), nodes.Get(3));


  // Install NDN stack on all nodes
  ndn::StackHelper ndnHelper;
  //ndnHelper.SetDefaultRoutes(true);
  ndnHelper.InstallAll();

  // Choosing forwarding strategy
  //`ndn::StrategyChoiceHelper::InstallAll("/prefix", "/localhost/nfd/strategy/ncc");
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  ndnGlobalRoutingHelper.Install(nodes);

  // Installing applications
 std::string strcallback;

// ns3::ndn::Synchronizer sync;//= new  ns3::ndn::Synchronizer();
 sync.setTimeStep(1.0);
 //sync.numberOfPackets(1);
 // Consumer
  ndn::AppHelper consumerHelper("ns3::ndn::ConsumerQos");
//  ndn::AppHelper consumerHelper("ns3::ndn::Subscriber");

  // Consumer will request /prefix/0, /prefix/1, ...
  consumerHelper.SetPrefix("/prefix");
  consumerHelper.SetAttribute("Frequency", StringValue("10")); // 10 interests a second
  consumerHelper.Install(nodes.Get(0));                        // first node
  strcallback = "/NodeList/" + std::to_string( 0 ) + "/ApplicationList/" + "*/ReceivedData";
  Config::ConnectWithoutContext( strcallback, MakeCallback( &ReceivedDataCallback ) );
  auto apps = nodes.Get(0)->GetApplication(0)->GetObject<ns3::ndn::ConsumerQos>();
  sync.addSender(0,apps);
  consumerHelper.Install(nodes.Get(1));                        // first node
  strcallback = "/NodeList/" + std::to_string( 1 ) + "/ApplicationList/" + "*/ReceivedData";
  Config::ConnectWithoutContext( strcallback, MakeCallback( &ReceivedDataCallback ) );
  apps = nodes.Get(1)->GetApplication(0)->GetObject<ns3::ndn::ConsumerQos>();
  sync.addSender(1,apps);
  consumerHelper.Install(nodes.Get(2));                        // first node
  strcallback = "/NodeList/" + std::to_string( 2 ) + "/ApplicationList/" + "*/ReceivedData";
  Config::ConnectWithoutContext( strcallback, MakeCallback( &ReceivedDataCallback ) );
  apps = nodes.Get(2)->GetApplication(0)->GetObject<ns3::ndn::ConsumerQos>();
  sync.addSender(2,apps);

  sync.beginSync();

  // Producer
  ndn::AppHelper producerHelper("ns3::ndn::Producer");
  // Producer will reply to all requests starting with /prefix
  producerHelper.SetPrefix("/prefix");
  producerHelper.SetAttribute("PayloadSize", StringValue("1024"));
  producerHelper.Install(nodes.Get(4)); // last node
  strcallback = "/NodeList/" + std::to_string( 4 ) + "/ApplicationList/" + "*/ReceivedInterest";
  Config::ConnectWithoutContext( strcallback, MakeCallback( &ReceivedInterestCallback ) );

  ndnGlobalRoutingHelper.AddOrigins("/prefix/", nodes.Get(4));

  ndn::GlobalRoutingHelper::CalculateRoutes();

  Simulator::Stop(Seconds(20.0));

  Simulator::Run();
  Simulator::Destroy();

  return 0;
}

void ReceivedInterestCallback( uint32_t nodeid, shared_ptr<const ndn::Interest> interest ){
	std::cout<< nodeid << ", " << interest->getName() << ", " << std::fixed << setprecision( 9 )
      << ( Simulator::Now().GetNanoSeconds() )/1000000000.0 << std::endl;
}
void ReceivedDataCallback( uint32_t nodeid, shared_ptr<const ndn::Data> data ){
        std::string str = std::to_string(nodeid) + ", " + data->getName().toUri() + ", " + std::to_string(( Simulator::Now().GetNanoSeconds() )/1000000000.0);
        sync.addArrivedPackets(str);      
}



} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}
