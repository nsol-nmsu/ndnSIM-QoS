/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright ( C ) 2020 New Mexico State University- Board of Regents
 *
 * George Torres, Anju Kunnumpurathu James
 * See AUTHORS.md for complete list of authors and contributors.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * ( at your option ) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/ndnSIM/helper/ndn-link-control-helper.hpp"
#include "ns3/ndnSIM/helper/ndn-fib-helper.hpp"

#include "apps/ndn-synchronizer-DDoS.hpp"
#include "nlohmann/json.hpp"


#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdlib.h>

#include <unordered_map>

#include "apps/ndn-app.hpp" //this header is required for Trace Sink

#define DEBUG 0

namespace ns3 {

using json = nlohmann::json;    // Library used for json parsing.

void SentInterestCallbackPhy( uint32_t, shared_ptr<const ndn::Interest> );
void ReceivedInterestCallbackPhy( uint32_t, shared_ptr<const ndn::Interest> );
void ReceivedInterestCallbackCom( uint32_t, shared_ptr<const ndn::Interest> );

// Split a line into a vector of strings
std::vector<std::string> SplitString( std::string );

// Get source node ID from which payload interest came from
uint32_t GetSourceNodeID ( std::string name );

// Check if source/destination flow is permitted in config file ( one-to-many, many-to-one flows )

// Store unique IDs to prevent repeated installation of app
bool IsPMUAppInstalled( std::string PMUID );
bool IsLCAppInstalled( std::string LCID );
bool IsWACAppInstalled( std::string WACID );

std::vector<std::string> uniquePMUs;
std::vector<std::string> uniqueLCs;
std::vector<std::string> uniqueWACs;

std::pair<int,int> pmu_nodes;
std::pair<int,int> lc_nodes;
std::pair<int,int> wac_nodes;

std::vector<std::pair<int,int>> all_flows;

// Vectors to store the various node types
vector<int> com_nodes, agg_nodes, phy_nodes;

// Vectors to store source and destination edges to fail
vector<int> srcedge, dstedge;

// Trace file
std::ofstream tracefile;
std::ofstream flowfile;

ns3::ndn::SyncDDoS sync;

int
main( int argc, char* argv[] )
{
  int run = 0;	

  // Read optional command-line parameters ( e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;
  cmd.AddValue("Run", "Run", run);
  cmd.Parse( argc, argv );

  sync.setTimeStep( 1.0 );                // Each time step is 5 seconds.

  // Open the configuration file for reading
  ifstream configFile ( "../topology/interface/case123.txt", std::ios::in );
  ifstream jsonFile ( "../topology/interface/data123.txt", std::ios::in );       // Device - Node mapping

  json jf = json::parse( jsonFile );
  json::iterator it = jf.begin();

  unordered_map<int,std::vector<std::string>> nameMap;

  // Debug print
  while ( it !=  jf.end() )
  {
     nameMap[ it.value() ].push_back( it.key() );
     //std::cout << it.key() << " : " << it.value() << "\n";
     it++;
  }

  sync.fillNameMap( jf );
  
  std::string strLine;
  bool gettingNodeCount = false, buildingNetworkTopo = false, attachingWACs = false, attachingPMUs = false, attachingLCs = false, TypeI = false, TypeII = false;
  bool failLinks = false, injectData = false, Sync = false;
  std::vector<std::string> netParams;

  NodeContainer nodes;
  int nodeCount = 0;
  std::pair<int,int> flow_pair;
  int beNodeCount = 1;
  std::string beNamePrefix;
  unordered_map<int,int> used;
  unordered_map<int,int> usedS;
  unordered_map<int,int> usedR;
  unordered_map<int,int> usedA;


  // Setting default parameters for PointToPoint links and channels
  // Config::SetDefault( "ns3::DropTailQueue::MaxPackets", StringValue( "10" ) );
  Config::SetDefault("ns3::QueueBase::MaxSize", StringValue("1500p"));


  PointToPointHelper p2p;

  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  std::string strcallback;

  ndn::AppHelper producerHelper( "ns3::ndn::QoSProducer" );
  ndn::AppHelper consumerHelper( "ns3::ndn::QoSConsumer" );
  ndn::AppHelper syncHelper( "ns3::ndn::ConsumerQos");

  flowfile.open( "ndn_all_flows.csv", std::ios::out );

  srand( run );

  int t1=0, t2=0, t3=0; 
  if ( configFile.is_open () ) {

    while ( std::getline( configFile, strLine ) ) {

      // Determine what operation is ongoing while reading the config file
      if( strLine.substr( 0,7 ) == "BEG_000" ) { gettingNodeCount = true; continue; }
      if( strLine.substr( 0,7 ) == "END_000" ) {
        // Create nodes
        nodeCount++; // Counter increment needed because nodes start from 1 not 0
        gettingNodeCount = false;
        nodes.Create( nodeCount );

        continue; 
      }
      if( strLine.substr( 0,7 ) == "BEG_001" ) { buildingNetworkTopo = true; continue; }
      if( strLine.substr( 0,7 ) == "END_001" ) { buildingNetworkTopo = false; /*continue; }
      if( strLine.substr( 0,7 ) == "BEG_002" ) { attachingWACs = false; continue; }
      if( strLine.substr( 0,7 ) == "END_002" ) { attachingWACs = false; continue; }
      if( strLine.substr( 0,7 ) == "BEG_003" ) { attachingPMUs = false; continue; }
      if( strLine.substr( 0,7 ) == "END_003" ) { attachingPMUs = false; continue; }
      if( strLine.substr( 0,7 ) == "BEG_004" ) { attachingLCs = true; continue; }
      if( strLine.substr( 0,7 ) == "END_004" ) {*/ 
        attachingLCs = false; 
        ndn::StackHelper ndnHelper; 
        ndnHelper.InstallAll();

        ndnGlobalRoutingHelper.Install( nodes );

        continue; 
      }
      if( strLine.substr( 0,7 ) == "BEG_505" ) { Sync = true; continue; }
      if( strLine.substr( 0,7 ) == "END_505" ) { Sync = false; uniqueLCs.clear(); continue; }      
      if( strLine.substr( 0,7 ) == "BEG_005" ) { //TypeI = true; 
	      continue; }
      if( strLine.substr( 0,7 ) == "END_005" ) { TypeI = false; uniquePMUs.clear(); continue; }
      if( strLine.substr( 0,7 ) == "BEG_006" ) { //TypeII = true; 
	      continue; }
      if( strLine.substr( 0,7 ) == "END_006" ) { TypeII = false; uniqueWACs.clear();  uniquePMUs.clear(); continue;}
      if( strLine.substr( 0,7 ) == "BEG_100" ) { failLinks = true; continue; }
      if( strLine.substr( 0,7 ) == "END_100" ) { failLinks = false; continue; }
      if( strLine.substr( 0,7 ) == "BEG_101" ) { //injectData = true; 
	      continue; }
      if( strLine.substr( 0,7 ) == "END_101" ) { injectData = false; continue; }


      if( gettingNodeCount == true ) {
        //Getting number of nodes to create
        netParams = SplitString( strLine );
        nodeCount = stoi( netParams[1] );

        //Store node ID ranges for various node types
      } 
      else if( buildingNetworkTopo == true ) {

        // Building network topology
        netParams = SplitString( strLine );
	int mbps = std::stoi(netParams[2])*100;
        p2p.SetDeviceAttribute( "DataRate", StringValue( std::to_string(mbps)+"Mbps" ) );
        p2p.SetChannelAttribute( "Delay", StringValue( netParams[3]+"ms" ) );
        p2p.Install( nodes.Get( std::stoi( netParams[0] ) ), nodes.Get( std::stoi( netParams[1] ) ) );
      } else if( attachingWACs == true ) {

        // Attaching WACs to routers
        netParams = SplitString( strLine );
        p2p.SetDeviceAttribute( "DataRate", StringValue( netParams[2]+"Mbps" ) );
        p2p.SetChannelAttribute( "Delay", StringValue( netParams[3]+"ms" ) );
        p2p.Install( nodes.Get( std::stoi( netParams[0] ) ), nodes.Get( std::stoi( netParams[1] ) ) );
      } else if( attachingPMUs == true ) {

        // Attaching PMUs to routers
        netParams = SplitString( strLine );
        p2p.SetDeviceAttribute( "DataRate", StringValue( netParams[2]+"Mbps" ) );
        p2p.SetChannelAttribute( "Delay", StringValue( netParams[3]+"ms" ) );
        p2p.Install( nodes.Get( std::stoi( netParams[0] ) ), nodes.Get( std::stoi( netParams[1] ) ) );
      } else if( attachingLCs == true ) {

        // Attaching LCs to routers
        netParams = SplitString( strLine );
        p2p.SetDeviceAttribute( "DataRate", StringValue( netParams[2]+"Mbps" ) );
        p2p.SetChannelAttribute( "Delay", StringValue( netParams[3]+"ms" ) );
        p2p.Install( nodes.Get( std::stoi( netParams[0] ) ), nodes.Get( std::stoi( netParams[1] ) ) );
      } 
      else if(Sync == true) {
         // Install apps on LCs and PMUs for data exchange
         netParams = SplitString( strLine );

         // Install app on unique LC WACs  
	 std::cout<<strLine<<std::endl;
	 if ( IsLCAppInstalled( netParams[0] ) == false ) {

 	    char temp[10];
	    sprintf( temp, "%lf", 1.0/( float( rand()%40+80 ) ) );
	    if ( DEBUG) std::cout << netParams[1] << "\n";

	    // Install consumer
	    syncHelper.SetPrefix( "/power/typeII" );
	    syncHelper.SetAttribute( "Subscription", IntegerValue( 0 ) );
	    syncHelper.SetAttribute( "PayloadSize", StringValue( "200" ) );
	    syncHelper.SetAttribute( "RetransmitPackets", IntegerValue( 0 ) );
	    syncHelper.SetAttribute( "LifeTime", StringValue( "100ms" ) );
	    syncHelper.Install( nodes.Get( std::stoi( netParams[0] ) ) );
	    usedS[std::stoi( netParams[0] )] = 1;

	    auto apps = nodes.Get( std::stoi( netParams[0] ) )->GetApplication( 0 )->GetObject<ns3::ndn::ConsumerQos>();
	    sync.addSender( std::stoi( netParams[0] ),apps );

	    // Install producer
	    producerHelper.SetPrefix( "/power/typeI/data" );
	    producerHelper.SetAttribute( "Frequency", StringValue( "0" ) );
	    producerHelper.Install( nodes.Get( std::stoi( netParams[0] ) ) );
	    producerHelper.SetPrefix( "/power/typeII/data" );
	    producerHelper.SetAttribute( "Frequency", StringValue( "0" ) );
	    producerHelper.Install( nodes.Get( std::stoi( netParams[0] ) ) );
	    used[std::stoi( netParams[0] )] = 1;
	    producerHelper.SetPrefix( "/power/typeIII/data" );
	    producerHelper.SetAttribute( "Frequency", StringValue( "0" ) );
	    producerHelper.Install( nodes.Get( std::stoi( netParams[0] ) ) );
	    used[std::stoi( netParams[0] )] = 1;
	    //usedS[std::stoi( netParams[0] )] = 1;

	    // Setup node to originate prefixes for dynamic routing
	    ndnGlobalRoutingHelper.AddOrigin( "/power/typeI/data", nodes.Get( std::stoi( netParams[0] ) ) );
	    ndnGlobalRoutingHelper.AddOrigin( "/power/typeII/data", nodes.Get( std::stoi( netParams[0] ) ) );
	    ndnGlobalRoutingHelper.AddOrigin( "/power/typeIII/data", nodes.Get( std::stoi( netParams[0] ) ) );

	    usedR[std::stoi( netParams[0] )] = 1;
	    sync.setPVNode(std::stoi( netParams[0] ));
	 }

	 // Install flow app on PMUs to send data to LCs
	 if ( IsPMUAppInstalled( netParams[1] ) == false ){
  	    char temp[10];
	    sprintf( temp, "%lf", 1.0/( float( rand()%40+80 ) ) );
	    std::string pre = "";
	    for(int i = 0; i < nameMap[std::stoi( netParams[1] )].size(); i++ ) {
 	       std::size_t found = nameMap[std::stoi( netParams[1] )][i].find("SW");
	       std::string com_string = nameMap[std::stoi( netParams[1] )][i].substr(0,1);
	       if (found!=std::string::npos || com_string == "V" || com_string == "R"){
 	          pre = "/power/typeI";
		  continue;
	       }
	       if( (com_string == "B" || com_string == "P") && pre != "/power/typeI" )            
	          pre = "/power/typeII";
	       if( (com_string == "C" || com_string == "L" || com_string == "T") && (pre != "/power/typeI" && pre != "/power/typeII"))
                  pre = "/power/typeIII";
	    }
	    if(pre == "") pre = "/power/typeII";
        
    	    if(pre == "/power/typeI" )
 	       t1++;
	    if(pre == "/power/typeII" )
               t2++;        
       	    if(pre == "/power/typeIII" )
               t3++;


	    syncHelper.SetPrefix( pre );
	    syncHelper.SetAttribute( "LeadPrefix", StringValue("/power/typeII") );
	    syncHelper.SetAttribute( "Subscription", IntegerValue( 0 ) );
	    syncHelper.SetAttribute( "PayloadSize", StringValue( "200" ) );
	    syncHelper.SetAttribute( "RetransmitPackets", IntegerValue( 0 ) );
	    syncHelper.SetAttribute( "LifeTime", StringValue( "1000ms" ) );

	    syncHelper.Install( nodes.Get( std::stoi( netParams[1] ) ) );
	    used[std::stoi( netParams[1] )] = 1;
	    //Config::ConnectWithoutContext( strcallback, MakeCallback( &SentInterestCallbackPhy ) );
	    usedS[std::stoi( netParams[1] )] = 1;
	    //std::cout << "installed on " << netParams[1] <<std::endl;
	    auto apps = nodes.Get( std::stoi( netParams[1] ) )->GetApplication( 0 )->GetObject<ns3::ndn::ConsumerQos>();

	    sync.addSender( std::stoi( netParams[1] ),apps );

	    // Setup node to originate prefixes for dynamic routing
	    for(int i = 0; i < nameMap[std::stoi( netParams[1] )].size(); i++ ) {
	       producerHelper.SetPrefix( "/power/typeI/phy"+ netParams[1] );
	       producerHelper.SetAttribute( "Frequency", StringValue( "0" ) );
	       producerHelper.Install( nodes.Get( std::stoi( netParams[1] ) ) );

	       producerHelper.SetPrefix( "/power/typeII/phy"+ netParams[1] );
	       producerHelper.SetAttribute( "Frequency", StringValue( "0" ) );
	       producerHelper.Install( nodes.Get( std::stoi( netParams[1] ) ) );

	       if ( DEBUG) std::cout << "Adding origin " << "/power/typeI/phy" + netParams[1] + "/" + nameMap[std::stoi( netParams[1] )][i] << " at node " << netParams[1] << std::endl;

	       std::string com_string = nameMap[std::stoi( netParams[1] )][i].substr(0,2);

	       if(com_string == "BE" || com_string == "PV"){
	          std::cout<<"/power/typeI/phy"<< netParams[1]<<"/"<<nameMap[std::stoi( netParams[1] )][i]<<std::endl;
		  //ndnGlobalRoutingHelper.AddOrigin( "/power/typeI/phy" + netParams[1] + "/" +
  		  //		  nameMap[std::stoi( netParams[1] )][i], nodes.Get( std::stoi( netParams[1] ) ) );
		  ndnGlobalRoutingHelper.AddOrigin( "/power/typeII/phy" + netParams[1] + "/" +
  		  		  nameMap[std::stoi( netParams[1] )][i], nodes.Get( std::stoi( netParams[1] ) ) );
	       }
	       usedR[std::stoi( netParams[1] )] = 2;
	    }

	    //ndnGlobalRoutingHelper.AddOrigin( "/power/typeII/data", nodes.Get( std::stoi( netParams[1] ) ) );
	    //usedR[std::stoi( netParams[1] )] = 1;

	 }

	 // Save the flow
	 flow_pair.first = stoi( netParams[0] );
	 flow_pair.second = stoi( netParams[1] );
	 all_flows.push_back( flow_pair );

	 // Write flow to file
	 flowfile << netParams[0] << " " << netParams[1] << " TypeI" << std::endl;
	 flowfile << netParams[0] << " " << netParams[1] << " TypeII" << std::endl;
	 flowfile << netParams[0] << " " << netParams[1] << " TypeIII" << std::endl;
      }
      else if( TypeI == true ) {

        // Install apps on LCs and PMUs for data exchange
        netParams = SplitString( strLine );

        // Install app on unique LC WACs
        if ( IsLCAppInstalled( netParams[0] ) == false ) {
          producerHelper.SetPrefix( "/power/typeI/back/dat"+netParams[0] );
          producerHelper.SetAttribute( "Frequency", StringValue( "0" ) );
	  std::cout<< netParams[0] <<std::endl;
          producerHelper.Install( nodes.Get( std::stoi( netParams[0] ) ) );
          used[std::stoi( netParams[0] )] = 1;
          // Setup node to originate prefixes for dynamic routing
          ndnGlobalRoutingHelper.AddOrigin( "/power/typeI/back/dat"+netParams[0], nodes.Get( std::stoi( netParams[0] ) ) );

          usedR[std::stoi( netParams[0] )] = 1;
        }

        // Install flow app on PMUs to send data to LCs
        if( IsPMUAppInstalled( netParams[1] ) == false ) {
          char temp[10];

          sprintf( temp, "%lf", 1.0/( float( rand()%10+20 ) ) );
          consumerHelper.SetPrefix( "/power/typeI/back/dat"+netParams[0]+"/phy" + netParams[1] );
          consumerHelper.SetAttribute( "Frequency", StringValue( temp ) ); //0.016 or 0.02
          consumerHelper.SetAttribute( "Subscription", IntegerValue( 0 ) );
          consumerHelper.SetAttribute( "PayloadSize", StringValue( "200" ) );
          consumerHelper.SetAttribute( "RetransmitPackets", IntegerValue( 0 ) );
          consumerHelper.SetAttribute( "Offset", IntegerValue( 0 ) );
          consumerHelper.SetAttribute( "LifeTime", StringValue( "1s" ) );
          consumerHelper.Install( nodes.Get( std::stoi( netParams[1] ) ) );
          used[std::stoi( netParams[1] )] = 1;
          usedS[std::stoi( netParams[1] )] = 1;

        }

        // Save the flow
        flow_pair.first = stoi( netParams[0] );
        flow_pair.second = stoi( netParams[1] );
        all_flows.push_back( flow_pair );

        // Write flow to file
        flowfile << netParams[0] << " " << netParams[1] << " TypeI" << std::endl;
      } else if( TypeII == true ) {

        // Install apps on LCs and PMUs for type II data exchange
        netParams = SplitString( strLine );

        // Install app on unique LC IDs(WACs)
        if ( IsWACAppInstalled( netParams[0] ) == false ) {
          producerHelper.SetPrefix( "/power/typeII/back/dat"+netParams[0] );
          producerHelper.SetAttribute( "Frequency", StringValue( "0" ) );
          producerHelper.Install( nodes.Get( std::stoi( netParams[0] ) ) );
          used[std::stoi( netParams[0] )] = 1;
          usedR[std::stoi( netParams[0] )] = 1;

          // Setup node to originate prefixes for dynamic routing
          ndnGlobalRoutingHelper.AddOrigin( "/power/typeII/back/dat"+netParams[0], nodes.Get( std::stoi( netParams[0] ) ) );
        }

        // Install flow app on WACs to send data to LCs
        if ( IsPMUAppInstalled( netParams[1] ) == false ) { 
          char temp[10];

          sprintf( temp, "%lf", 1.0/( float( rand()%20+50 ) ) ); //========= AC -> LC (Type II - 120/sec),  give the avg (100 - 140)
          //std::cout<<netParams[1]<<std::endl;

          consumerHelper.SetPrefix( "/power/typeII/back/dat"+netParams[0]+"/phy" + netParams[1] );
          consumerHelper.SetAttribute( "Frequency", StringValue( temp ) ); //0.016 or 0.02
          consumerHelper.SetAttribute( "Subscription", IntegerValue( 0 ) );
          consumerHelper.SetAttribute( "PayloadSize", StringValue( "200" ) );
          consumerHelper.SetAttribute( "RetransmitPackets", IntegerValue( 0 ) );
          consumerHelper.SetAttribute( "Offset", IntegerValue( 0 ) );
          consumerHelper.SetAttribute( "LifeTime", StringValue( "1s" ) );
          //consumerHelper.Install( nodes.Get( std::stoi( netParams[1] ) ) );
          //used[std::stoi( netParams[1] )] = 1;
          //usedS[std::stoi( netParams[1] )] = 1;

	} 

        // Save the flow
        flow_pair.first = stoi( netParams[0] );
        flow_pair.second = stoi( netParams[1] );
        all_flows.push_back( flow_pair );

        // Write flow to file
        flowfile << netParams[0] << " " << netParams[1] << " TypeII" << std::endl;
      } else if( failLinks == true ) {

        // Schedule the links to fail
        netParams = SplitString( strLine );

        Simulator::Schedule( Seconds(  ( ( double )stod( netParams[2] ) )  ), ndn::LinkControlHelper::FailLink, nodes.Get( stoi( netParams[0] ) ), nodes.Get( stoi( netParams[1] ) ) );
        Simulator::Schedule( Seconds(  ( ( double )stod( netParams[3] ) )  ), ndn::LinkControlHelper::UpLink, nodes.Get( stoi( netParams[0] ) ), nodes.Get( stoi( netParams[1] ) ) );

      } else if( injectData == true ) {
        
	/*
        netParams = SplitString( strLine );
	 
        // Install app on target node for data injection
        beNamePrefix = "/power/typeIII/back/dat" + netParams[0];
	if ( IsWACAppInstalled( netParams[0] ) == false ) {
        producerHelper.SetPrefix( beNamePrefix );
        producerHelper.SetAttribute( "Frequency", StringValue( "0" ) );
        producerHelper.Install( nodes.Get( std::stoi( netParams[0] ) ) );
        used[std::stoi( netParams[0] )] = 1;
        ndnGlobalRoutingHelper.AddOrigin( beNamePrefix, nodes.Get( std::stoi( netParams[0] ) ) );
	usedR[std::stoi( netParams[0] )] = 1;
	}

        char temp[10];
        sprintf( temp, "%lf", 1.0/( float( rand()%20+100 ) ) );   // No type III. So 0.0 is used.

        consumerHelper.SetPrefix( beNamePrefix + "/ph" + netParams[1] );
        consumerHelper.SetAttribute( "Frequency", StringValue( temp ) ); //0.0002 = 5000pps
        consumerHelper.SetAttribute( "Subscription", IntegerValue( 0 ) );
        consumerHelper.SetAttribute( "PayloadSize", StringValue( "1024" ) );
        consumerHelper.SetAttribute( "RetransmitPackets", IntegerValue( 0 ) );
        consumerHelper.SetAttribute( "Offset", IntegerValue( 0 ) );
        consumerHelper.SetAttribute( "LifeTime", StringValue( "1s" ) );
        consumerHelper.Install( nodes.Get( std::stoi( netParams[1] ) ) );

        used[std::stoi( netParams[1] )] = 1;
        flow_pair.first = stoi( netParams[0] );
        flow_pair.second = stoi( netParams[1] );
        all_flows.push_back( flow_pair );
        flowfile << netParams[0] << " " << netParams[1] << " TypeIII" << std::endl;
        beNodeCount += 1;
        usedS[std::stoi( netParams[1] )] = 1; */

           netParams = SplitString( strLine );
	   consumerHelper.SetPrefix( "/" );
	   consumerHelper.SetAttribute( "Attacker", IntegerValue( 1 ) );
 	   consumerHelper.Install( nodes.Get( std::stoi( netParams[0] ) ) );

	   used[std::stoi( netParams[0] )] = 1;
	   usedA[std::stoi( netParams[0] )] = 1;
	   auto apps = nodes.Get( std::stoi( netParams[0] ) )->GetApplication( 0 )->GetObject<ns3::ndn::ConsumerQos>();
	   sync.addAttackers( std::stoi( netParams[0] ),apps );


      } else {
        //std::cout << "reading something else " << strLine << std::endl;
      }	

    } //end while
  } else {
    std::cout << "Cannot open configuration file!!!" << std::endl;
    exit( 1 );
  }

  configFile.close();
  std::cout<<"All done\n";

  // Define the routing strategies per prefix
  //ndn::StrategyChoiceHelper::InstallAll( "/power/typeI", "/localhost/nfd/strategy/multicast" );
  //ndn::StrategyChoiceHelper::InstallAll( "/power/typeII", "/localhost/nfd/strategy/multicast" );
  //ndn::StrategyChoiceHelper::InstallAll( "/power/typeIII", "/localhost/nfd/strategy/best-route" );
  //ndn::StrategyChoiceHelper::InstallAll( "/", "/localhost/nfd/strategy/qos" );
 

  ndn::AppHelper tokenHelper( "ns3::ndn::TokenBucketDriver" );
  tokenHelper.SetAttribute( "FillRates", StringValue( "10000 15000 10000 350" ) ); // 10 interests a second
  tokenHelper.SetAttribute( "Capacities", StringValue( "200 200 200 200" ) ); // 10 interests a second



  bool icaap = false;
   for ( int i=0; i<( int )nodes.size(); i++ ) {
    if( used[i] == 0 && !icaap){
      ndn::StrategyChoiceHelper::Install( nodes.Get( i ),"power/typeI", "/localhost/nfd/strategy/qos" );
      ndn::StrategyChoiceHelper::Install( nodes.Get( i ),"power/typeII", "/localhost/nfd/strategy/qos" );
      ndn::StrategyChoiceHelper::Install( nodes.Get( i ),"power/typeIII", "/localhost/nfd/strategy/qos" );
      ndn::StrategyChoiceHelper::Install( nodes.Get( i ),"pow/typeI", "/localhost/nfd/strategy/qos" );
      ndn::StrategyChoiceHelper::Install( nodes.Get( i ),"pow/typeII", "/localhost/nfd/strategy/qos" );
      ndn::StrategyChoiceHelper::Install( nodes.Get( i ),"pow/typeIII", "/localhost/nfd/strategy/qos" );

      //ndn::StrategyChoiceHelper::Install( nodes.Get( i ),"/", "/localhost/nfd/strategy/qos" );

      tokenHelper.Install( nodes.Get( i ) );
    }

    else if(usedA[i] == 1){
       //std::cout<<"installing attacker strategy choice helper"<<std::endl; 
       ndn::StrategyChoiceHelper::Install( nodes.Get( i ),"power/typeI", "/localhost/nfd/strategy/best-route-attacker" );
       ndn::StrategyChoiceHelper::Install( nodes.Get( i ),"power/typeII", "/localhost/nfd/strategy/best-route-attacker" );
       ndn::StrategyChoiceHelper::Install( nodes.Get( i ),"power/typeIII", "/localhost/nfd/strategy/best-route-attacker" );
    }
   }



  /*for ( int i=0; i<( int )nodes.size(); i++ ) {
    if( used[i] == 0 && icaap){
      ndn::StrategyChoiceHelper::Install( nodes.Get( i ),"power/typeI", "/localhost/nfd/strategy/icaap" );
      ndn::StrategyChoiceHelper::Install( nodes.Get( i ),"power/typeII", "/localhost/nfd/strategy/icaap" );
      ndn::StrategyChoiceHelper::Install( nodes.Get( i ),"power/typeIII", "/localhost/nfd/strategy/icaap" );
      ndn::StrategyChoiceHelper::Install( nodes.Get( i ),"pow/typeI", "/localhost/nfd/strategy/icaap" );
      ndn::StrategyChoiceHelper::Install( nodes.Get( i ),"pow/typeII", "/localhost/nfd/strategy/icaap" );
      ndn::StrategyChoiceHelper::Install( nodes.Get( i ),"pow/typeIII", "/localhost/nfd/strategy/icaap" );

      //ndn::StrategyChoiceHelper::Install( nodes.Get( i ),"/", "/localhost/nfd/strategy/qos" );

      //tokenHelper.Install( nodes.Get( i ) );
    }
  }*/

  // Populate routing table for nodes
  //ndn::GlobalRoutingHelper::CalculateRoutes();
  ndn::GlobalRoutingHelper::CalculateAllPossibleRoutes();
  sync.beginSync();
  std::cout<<"Routes calculated\n";

  //Open trace file for writing
  char trace[100];
  if(icaap)
     sprintf( trace, "ndn-case1-run%d.csv", run );
  else 
     sprintf( trace, "ndn-case-icaap++%d.csv", run);

  tracefile.open( trace, std::ios::out );
  tracefile << "nodeid, event, name, payloadsize, time" << std::endl;
  for ( int i=0; i<( int )nodes.size(); i++ ) {
    if( usedS[i] == 1 ) {
      strcallback = "/NodeList/" + std::to_string( i ) + "/ApplicationList/" + "*/SentInterest";
      Config::ConnectWithoutContext( strcallback, MakeCallback( &SentInterestCallbackPhy ) );
    }
  }

  for ( int i=0; i<( int )nodes.size(); i++ ) {
    if( usedR[i] == 1 ) {
      strcallback = "/NodeList/" + std::to_string( i ) + "/ApplicationList/" + "*/ReceivedInterest";
      Config::ConnectWithoutContext( strcallback, MakeCallback( &ReceivedInterestCallbackCom ) );
    }
    else if (usedR[i] == 2) {
		    strcallback = "/NodeList/" + std::to_string( i ) + "/ApplicationList/" + "*/ReceivedInterest";
		          Config::ConnectWithoutContext( strcallback, MakeCallback( &ReceivedInterestCallbackPhy ) );
			  }		    
  }

  Simulator::Stop( Seconds( 10800.0 ) );
  Simulator::Run();
  sync.sendSync();
  Simulator::Destroy();

  return 0;
}


// Get source node ID from which payload interest came from
uint32_t
GetSourceNodeID ( std::string name ) {

  // Find first occurence of "/" and get remaining string to its right
  std::size_t pos = name.find( "/" );
  std::string remstr = name.substr( pos+1,string::npos );

  // Find second occurence of "/" and get remaining string to its right
  pos = remstr.find( "/" );
  remstr = remstr.substr( pos+1,string::npos );

  // Find third occurence of "/" and get remaining string to its right
  pos = remstr.find( "/" );
  remstr = remstr.substr( pos+1,string::npos );

  // Find fourth occurence of "/" and get remaining string to its right
  pos = remstr.find( "/" );
  remstr = remstr.substr( pos+1,string::npos );

  // Find fifth occurence of "/" and get source node ID
  pos = remstr.find( "/" );
  remstr = remstr.substr( 3,pos-3 );

  return stoi( remstr );
}


// Split a string delimited by space
std::vector<std::string>
SplitString( std::string strLine ) {
  std::string str = strLine;
  std::vector<std::string> result;
  std::istringstream isstr( str );

  for( std::string str; isstr >> str;  )
    result.push_back( str );

  return result;
}

// Store unique PMU IDs to prevent repeated installation of app
bool
IsPMUAppInstalled( std::string PMUID ) {
  for ( int i=0; i<( int )uniquePMUs.size(); i++ ) {
    if( PMUID.compare( uniquePMUs[i] ) == 0 ) {
      return true;
    }
  }

  uniquePMUs.push_back( PMUID );

  return false;
}

// Store unique PDC IDs to prevent repeated installation of app
bool
IsLCAppInstalled( std::string LCID ) {
  for ( int i=0; i<( int )uniqueLCs.size(); i++ ) {
    if( LCID.compare( uniqueLCs[i] ) == 0 ) {
      return true;
    }
  }

  uniqueLCs.push_back( LCID );

  return false;
}

// Store unique WAC IDs to prevent repeated installation of app
bool
IsWACAppInstalled( std::string WACID ) {
  for ( int i=0; i<( int )uniqueWACs.size(); i++ ) {
    if( WACID.compare( uniqueWACs[i] ) == 0 ) {
      return true;
    }
  }

  uniqueWACs.push_back( WACID );

  return false;
}


// Define callbacks for writing to tracefile
void
SentInterestCallbackPhy( uint32_t nodeid, shared_ptr<const ndn::Interest> interest ) {

        if ( interest->getName( ).getSubName( 2,1 ).toUri( ).substr( 0,5 ) == "/back" ) {
		return;
	}
	if ( interest->getSubscription() == 1 || interest->getSubscription( ) == 2 ) {

                //Do not log subscription interests from phy nodes
        } else {

                tracefile << nodeid << ", sent, " << interest->getName() << ", " << interest->getPayloadLength( ) << ", " << std::fixed << setprecision( 9 )
                        << ( Simulator::Now().GetNanoSeconds( ) )/1000000000.0 << std::endl;
        }
}


//Callback for Transformers, switch, etc. All devices other than ReDis PV controller
//Checks to see if packet is a setpoint, or measument data.
void
ReceivedInterestCallbackPhy( uint32_t nodeid, shared_ptr<const ndn::Interest> interest ) {

	if ( interest->getName( ).getSubName( 2,1 ).toUri( ).substr( 0,5 ) == "/back" ) {
                //tracefile << nodeid << ", recv, " << interest->getName() << ", " << interest->getPayloadLength( ) << ", " << std::fixed << setprecision( 9 )
		//	                        << ( Simulator::Now().GetNanoSeconds( ) )/1000000000.0 << std::endl;
		return;
	}
        //Check to see if packet is from a follower
        //If so aggregate the data at the lead
	else if ( interest->getName( ).getSubName( -3,1 ).toUri( ).substr( 1 ) == "data" ) {

                //if ( DEBUG)
                //std::cout<<"We got it "<< interest->getName( ).toUri()<<std::endl;

                // Do not log subscription interests received at com nodes
                if ( interest->getName().getSubName( 1,1 ).toUri( ).substr(0,5) == "/type" &&
                        interest->getName().getSubName( 2,1 ).toUri( ).substr(0,4) == "/phy" ){

                        std::vector<uint8_t> payloadVector( &interest->getPayload()[0], &interest->getPayload()[interest->getPayloadLength()] );
                        std::string payload( payloadVector.begin(), payloadVector.end() );
                        std::string str = /*std::to_string( nodeid )*/std::to_string( interest->getPayloadLength() ) + " " + interest->getName( ).getSubName( -2,1 ).toUri( ).substr( 1 ) + " " + std::to_string( ( Simulator::Now( ).GetSeconds( ) ) ) + " OpenDSS "+payload ;

                        sync.aggDER( payload, nodeid,  interest->getName().getSubName( -2,1 ).toUri().substr( 1 ), interest->getName().getSubName( 3,1 ).toUri().substr( 1 ) );
                        if ( DEBUG) std::cout << "Received: " << str <<std::endl;
                }
                else {
                   std::cout<<"Not used phy "<<interest->getName()<<std::endl;
                }
                tracefile << nodeid << ", recv, " << interest->getName() << ", " << interest->getPayloadLength( ) << ", " << std::fixed << setprecision( 9 )
                        << ( Simulator::Now().GetNanoSeconds( ) )/1000000000.0 << std::endl;

        }
        //Otherwise the packet is a setpoint, check to see if it is a lead or not.
        //If it is not update sync with arriving packets data
        else {
                //std::cout<<Simulator::Nowg.GetSeconds( )<<std::endl;

                if ( interest->getName().getSubName( 1,1 ).toUri( ).substr(0,5) == "/type"){

                        std::vector<uint8_t> payloadVector( &interest->getPayload()[0], &interest->getPayload()[interest->getPayloadLength()] );
                        std::string payload( payloadVector.begin(), payloadVector.end() );
                        std::string str = std::to_string( interest->getPayloadLength() ) + " "
                                + interest->getName( ).getSubName( 3,1 ).toUri( ).substr( 1 ) + " "
                                + std::to_string( ( Simulator::Now( ).GetSeconds( ) ) ) + " RedisPV " + payload;

                        //std::cout<< "\nReceived Setpoint: " << interest->getName() << "   " <<payload <<std::endl;
                        bool lead = sync.sendDirect( payload, nodeid,  interest->getName().getSubName( 3,1 ).toUri().substr( 1 ) );

                        if( !lead ) {
                                sync.addArrivedPackets( str );
                        }


                        //std::cout << "Check string: " << str << std::endl;

                }

                //Only log flows that are permitted in config file ( eliminates redundant interests received from multiple interfaces and to other nodes )
                //if ( FlowPermitted( ( int )nodeid, ( int )GetSourceNodeID( iname ) ) == true ) {
                tracefile << nodeid << ", recv, " << interest->getName() << ", " << interest->getPayloadLength( ) << ", " << std::fixed << setprecision( 9 )
                        << ( Simulator::Now().GetNanoSeconds( ) )/1000000000.0 << std::endl;
                //}
        }
}



void
ReceivedInterestCallbackCom( uint32_t nodeid, shared_ptr<const ndn::Interest> interest ) {

   if ( interest->getName( ).getSubName( 2,1 ).toUri( ).substr( 0,5 ) == "/back" ) {
      //tracefile << nodeid << ", recv, " << interest->getName() << ", " << interest->getPayloadLength( ) << ", " << std::fixed << setprecision( 9 )
//	      << ( Simulator::Now().GetNanoSeconds( ) )/1000000000.0 << std::endl;
      return;
   }

   //else if ( interest->getSubscription() == 1 || interest->getSubscription( ) == 2 ) {
      // Do not log subscription interests received at com nodes
   //} 
   else {
      if ( interest->getName().getSubName( 1,1 ).toUri( ).substr(0,5) == "/type"){

         std::vector<uint8_t> payloadVector( &interest->getPayload()[0], &interest->getPayload()[interest->getPayloadLength()] );
	 std::string payload( payloadVector.begin(), payloadVector.end() );
	 std::string str = /*std::to_string( nodeid )*/std::to_string( interest->getPayloadLength() ) + " "
		 + interest->getName( ).getSubName( 3,1 ).toUri( ).substr( 1 ) + " "
		 + std::to_string( ( Simulator::Now( ).GetSeconds( ) ) ) + " OpenDSS "+payload;

	 sync.addArrivedPackets( str );
	 if ( DEBUG) std::cout<< "Received: " << str <<std::endl;
      }
      else {
         std::cout<<"Not used "<<interest->getName()<<std::endl;
      }
  
      //Only log flows that are permitted in config file ( eliminates redundant interests received from multiple interfaces and to other nodes )
      tracefile << nodeid << ", recv, " << interest->getName() << ", " << interest->getPayloadLength( ) << ", " << std::fixed << setprecision( 9 )
	      << ( Simulator::Now().GetNanoSeconds( ) )/1000000000.0 << std::endl;                
   }
}
}
int
main( int argc, char* argv[] )
{
  return ns3::main( argc, argv );
}
