/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright ( c ) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or ( at your option ) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/ndnSIM/helper/ndn-link-control-helper.hpp"
#include "ns3/ndnSIM/helper/ndn-fib-helper.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdlib.h>

#include <unordered_map>

#include "apps/ndn-app.hpp"		// This header is required for Trace Sink
#include "nlohmann/json.hpp"

#define DEBUG 0


namespace ns3 {

using json = nlohmann::json;	// Library used for json parsing.

void SentInterestCallbackPhy( uint32_t, shared_ptr<const ndn::Interest> );
void ReceivedInterestCallbackPhy( uint32_t, shared_ptr<const ndn::Interest> );
void ReceivedInterestCallbackCom( uint32_t, shared_ptr<const ndn::Interest> );

bool NodeInComm( int );
bool NodeInAgg( int );

std::vector<std::string> SplitString( std::string );	// Split a line into a vector of strings
uint32_t GetSourceNodeID ( std::string name );			// Get source node ID from which payload interest came from

// Check if source/destination flow is permitted in config file ( one-to-many, many-to-one flows )
bool FlowPermitted ( int dstnode, int srcnode );

// Store unique IDs to prevent repeated installation of app
bool IsPMUAppInstalled( std::string PMUID );
bool IsLCAppInstalled( std::string LCID );
bool IsWACAppInstalled( std::string WACID );

std::vector<std::string> uniquePMUs;
std::vector<std::string> uniqueLCs;
std::vector<std::string> uniqueWACs;
std::vector<std::pair<int,int>> all_flows;

std::pair<int,int> pmu_nodes;
std::pair<int,int> lc_nodes;
std::pair<int,int> wac_nodes;


vector<int> com_nodes, agg_nodes, phy_nodes;		// Vectors to store the various node types
vector<int> srcedge, dstedge;						// Vectors to store source and destination edges to fail
std::ofstream tracefile;							// Trace file
std::ofstream flowfile;

ns3::ndn::SyncSocket sync;


int
main ( int argc, char* argv[] )
{
	bool gettingNodeCount = false, buildingNetworkTopo = false, attachingWACs = false, attachingPMUs = false, attachingLCs = false, TypeI = false, TypeII = false;
	bool failLinks = false, injectData = false;

	int run = 0;	
	int nodeCount = 0;
	int beNodeCount = 1;

	std::string strLine;
	std::string beNamePrefix;

	NodeContainer nodes;

	std::pair<int,int> flow_pair;
	std::vector<std::string> netParams;

	unordered_map<int,int> used;
	unordered_map<int,int> usedS;
	unordered_map<int,int> usedR;

	sync.setTimeStep( 20.0 );		// Each time step is 20 seconds.

	// Read optional command-line parameters ( e.g., enable visualizer with ./waf --run=<> --visualize
	CommandLine cmd;
	cmd.AddValue( "Run", "Run", run );
	cmd.Parse( argc, argv );

	// Open the configuration files for reading -  TODO: Remove hard coding
	ifstream configFile ( "/home/anju/anju/NDN_QoS/topology/interface/case123.txt", std::ios::in );	// Topology file
	ifstream jsonFile ( "/home/anju/anju/NDN_QoS/topology/interface/data123.txt", std::ios::in );	// Device - Node mapping
	ifstream mFile ( "/home/anju/anju/NDN_QoS/topology/interface/measurments.json", std::ios::in );	// Basic measurement json to update duting simulation.

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
	sync.initializeJson( json::parse( mFile ) );

	PointToPointHelper p2p;

	ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
	std::string strcallback;

	ndn::AppHelper producerHelper( "ns3::ndn::SpontaneousProducer" );
	ndn::AppHelper consumerHelper( "ns3::ndn::ConsumerQos" );

	flowfile.open( "ndn_all_flows.csv", std::ios::out );
	srand( run );

	if ( configFile.is_open() ) {

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
			if( strLine.substr( 0,7 ) == "END_001" ) { buildingNetworkTopo = false; continue; }
			if( strLine.substr( 0,7 ) == "BEG_002" ) { attachingWACs = true; continue; }
			if( strLine.substr( 0,7 ) == "END_002" ) { attachingWACs = false; continue; }
			if( strLine.substr( 0,7 ) == "BEG_003" ) { attachingPMUs = true; continue; }
			if( strLine.substr( 0,7 ) == "END_003" ) { attachingPMUs = false; continue; }
			if( strLine.substr( 0,7 ) == "BEG_004" ) { attachingLCs = true; continue; }
			if( strLine.substr( 0,7 ) == "END_004" ) { 
				attachingLCs = false; 
				ndn::StackHelper ndnHelper; 
				ndnHelper.InstallAll();
				ndnGlobalRoutingHelper.Install( nodes );
				continue; 
			}
			if( strLine.substr( 0,7 ) == "BEG_005" ) { TypeI = true; continue; }
			if( strLine.substr( 0,7 ) == "END_005" ) { TypeI = false; uniquePMUs.clear(); continue; }
			if( strLine.substr( 0,7 ) == "BEG_006" ) { TypeII = true; continue; }
			if( strLine.substr( 0,7 ) == "END_006" ) { TypeII = false; continue; }
			if( strLine.substr( 0,7 ) == "BEG_100" ) { failLinks = true; continue; }
			if( strLine.substr( 0,7 ) == "END_100" ) { failLinks = false; continue; }
			if( strLine.substr( 0,7 ) == "BEG_101" ) { injectData = true; continue; }
			if( strLine.substr( 0,7 ) == "END_101" ) { injectData = false; continue; }


			if ( gettingNodeCount == true ) {

				// Getting number of nodes to create
				netParams = SplitString( strLine );
				nodeCount = stoi( netParams[1] );

			} else if ( buildingNetworkTopo == true ) {

				// Building network topology
				netParams = SplitString( strLine );
				p2p.SetDeviceAttribute( "DataRate", StringValue( netParams[2]+"Mbps" ) );
				p2p.SetChannelAttribute( "Delay", StringValue( netParams[3]+"ms" ) );
				p2p.Install( nodes.Get( std::stoi( netParams[0] ) ), nodes.Get( std::stoi( netParams[1] ) ) );

			} else if ( attachingWACs == true ) {

				// Attaching WACs to routers
				netParams = SplitString( strLine );
				p2p.SetDeviceAttribute( "DataRate", StringValue( netParams[2]+"Mbps" ) );
				p2p.SetChannelAttribute( "Delay", StringValue( netParams[3]+"ms" ) );
				p2p.Install( nodes.Get( std::stoi( netParams[0] ) ), nodes.Get( std::stoi( netParams[1] ) ) );

			} else if ( attachingPMUs == true ) {

				// Attaching PMUs to routers
				netParams = SplitString( strLine );
				p2p.SetDeviceAttribute( "DataRate", StringValue( netParams[2]+"Mbps" ) );
				p2p.SetChannelAttribute( "Delay", StringValue( netParams[3]+"ms" ) );
				p2p.Install( nodes.Get( std::stoi( netParams[0] ) ), nodes.Get( std::stoi( netParams[1] ) ) );

			} else if ( attachingLCs == true ) {

				// Attaching LCs to routers
				netParams = SplitString( strLine );
				p2p.SetDeviceAttribute( "DataRate", StringValue( netParams[2]+"Mbps" ) );
				p2p.SetChannelAttribute( "Delay", StringValue( netParams[3]+"ms" ) );
				p2p.Install( nodes.Get( std::stoi( netParams[0] ) ), nodes.Get( std::stoi( netParams[1] ) ) );

			} else if ( TypeI == true ) {

				// Install apps on LCs and PMUs for data exchange
				netParams = SplitString( strLine );

				// Install app on unique LC WACs
				if ( IsLCAppInstalled( netParams[0] ) == false ) {

					char temp[10];
					sprintf( temp, "%lf", 1.0/( float( rand()%40+80 ) ) );
					if ( DEBUG) std::cout << netParams[1] << "\n";

					// Install consumer
					consumerHelper.SetPrefix( "/power/typeI" );
					consumerHelper.SetAttribute( "Subscription", IntegerValue( 0 ) );
					consumerHelper.SetAttribute( "PayloadSize", StringValue( "200" ) );
					consumerHelper.SetAttribute( "RetransmitPackets", IntegerValue( 0 ) );
					consumerHelper.SetAttribute( "LifeTime", StringValue( "100ms" ) );
					consumerHelper.Install( nodes.Get( std::stoi( netParams[0] ) ) );
					used[std::stoi( netParams[0] )] = 1;
					//Config::ConnectWithoutContext( strcallback, MakeCallback( &SentInterestCallbackPhy ) );
					//usedS[std::stoi( netParams[0] )] = 1;

					auto apps = nodes.Get( std::stoi( netParams[0] ) )->GetApplication( 0 )->GetObject<ns3::ndn::ConsumerQos>();

					sync.addSender( std::stoi( netParams[0] ),apps );

					// Install producer
					producerHelper.SetPrefix( "/power/typeI/data" );
					producerHelper.SetAttribute( "Frequency", StringValue( "0" ) );
					producerHelper.Install( nodes.Get( std::stoi( netParams[0] ) ) );
					used[std::stoi( netParams[0] )] = 1;

					// Setup node to originate prefixes for dynamic routing
					ndnGlobalRoutingHelper.AddOrigin( "/power/typeI/data", nodes.Get( std::stoi( netParams[0] ) ) );
					usedR[std::stoi( netParams[0] )] = 1;
					sync.setPVNode(std::stoi( netParams[0] ));

				}

				// Install flow app on PMUs to send data to LCs
				if ( IsPMUAppInstalled( netParams[1] ) == false ){

					char temp[10];
					sprintf( temp, "%lf", 1.0/( float( rand()%40+80 ) ) );

					consumerHelper.SetPrefix( "/power/typeI" );
					consumerHelper.SetAttribute( "Subscription", IntegerValue( 0 ) );
					consumerHelper.SetAttribute( "PayloadSize", StringValue( "200" ) );
					consumerHelper.SetAttribute( "RetransmitPackets", IntegerValue( 0 ) );
					consumerHelper.SetAttribute( "LifeTime", StringValue( "100ms" ) );

					consumerHelper.Install( nodes.Get( std::stoi( netParams[1] ) ) );
					used[std::stoi( netParams[1] )] = 1;
					//Config::ConnectWithoutContext( strcallback, MakeCallback( &SentInterestCallbackPhy ) );
					usedS[std::stoi( netParams[1] )] = 1;

					auto apps = nodes.Get( std::stoi( netParams[1] ) )->GetApplication( 0 )->GetObject<ns3::ndn::ConsumerQos>();

					sync.addSender( std::stoi( netParams[1] ),apps );

					producerHelper.SetPrefix( "/power/typeI/phy"+ netParams[1] );
					producerHelper.SetAttribute( "Frequency", StringValue( "0" ) );
					producerHelper.Install( nodes.Get( std::stoi( netParams[1] ) ) );
					used[std::stoi( netParams[1] )] = 1;

					// Setup node to originate prefixes for dynamic routing
					for(int i = 0; i < nameMap[std::stoi( netParams[1] )].size(); i++ ) {

						if ( DEBUG) std::cout << "Adding origin " << "/power/typeI/phy" + netParams[1] + "/" + nameMap[std::stoi( netParams[1] )][i] << " at node " << netParams[1] << std::endl;
						ndnGlobalRoutingHelper.AddOrigin( "/power/typeI/phy" + netParams[1] + "/" + nameMap[std::stoi( netParams[1] )][i], nodes.Get( std::stoi( netParams[1] ) ) );
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

			} else if ( TypeII == true ) {

				// Install apps on LCs and PMUs for type II data exchange
				netParams = SplitString( strLine );

				// Install app on unique LC IDs( WACs )
				if ( IsWACAppInstalled( netParams[0] ) == false ) {

					producerHelper.SetPrefix( "/power/typeII/data" );
					producerHelper.SetAttribute( "Frequency", StringValue( "0" ) );

					producerHelper.Install( nodes.Get( std::stoi( netParams[0] ) ) );

					used[std::stoi( netParams[0] )] = 1;
					usedR[std::stoi( netParams[0] )] = 1;

					// Setup node to originate prefixes for dynamic routing
					ndnGlobalRoutingHelper.AddOrigin( "/power/typeII/data", nodes.Get( std::stoi( netParams[0] ) ) );
				}

				// Install flow app on WACs to send data to LCs
				if ( IsPMUAppInstalled( netParams[1] ) == false ) { 

					char temp[10];
					sprintf( temp, "%lf", 1.0/( float( rand()%40+100 ) ) );

					//std::cout<<netParams[1]<<std::endl;

					consumerHelper.SetPrefix( "/power/typeII/data/phy" + netParams[1] );
					consumerHelper.SetAttribute( "Frequency", StringValue( temp ) ); //0.016 or 0.02
					consumerHelper.SetAttribute( "Subscription", IntegerValue( 0 ) );
					consumerHelper.SetAttribute( "PayloadSize", StringValue( "200" ) );
					consumerHelper.SetAttribute( "RetransmitPackets", IntegerValue( 0 ) );
					consumerHelper.SetAttribute( "Offset", IntegerValue( 0 ) );
					consumerHelper.SetAttribute( "LifeTime", StringValue( "100ms" ) );

					consumerHelper.Install( nodes.Get( std::stoi( netParams[1] ) ) );
					used[std::stoi( netParams[1] )] = 1;
					//strcallback = "/NodeList/" + netParams[1] + "/ApplicationList/" + "*/SentInterest";
					//Config::ConnectWithoutContext( strcallback, MakeCallback( &SentInterestCallbackPhy ) );
					usedS[std::stoi( netParams[1] )] = 1;
				} 

				// Save the flow
				flow_pair.first = stoi( netParams[0] );
				flow_pair.second = stoi( netParams[1] );
				all_flows.push_back( flow_pair );

				// Write flow to file
				flowfile << netParams[0] << " " << netParams[1] << " TypeII" << std::endl;

			} else if ( failLinks == true ) {

				// Schedule the links to fail
				netParams = SplitString( strLine );

				Simulator::Schedule( Seconds(  ( ( double )stod( netParams[2] ) )  ), ndn::LinkControlHelper::FailLink, nodes.Get( stoi( netParams[0] ) ), nodes.Get( stoi( netParams[1] ) ) );
				Simulator::Schedule( Seconds(  ( ( double )stod( netParams[3] ) )  ), ndn::LinkControlHelper::UpLink, nodes.Get( stoi( netParams[0] ) ), nodes.Get( stoi( netParams[1] ) ) );

			} else if( injectData == true ) {

				netParams = SplitString( strLine );

				// Install app on target node for data injection
				beNamePrefix = "/power/be/dat" + std::to_string( beNodeCount );
				producerHelper.SetPrefix( beNamePrefix );
				producerHelper.SetAttribute( "Frequency", StringValue( "0" ) );
				//producerHelper.Install( nodes.Get( std::stoi( netParams[0] ) ) );
				used[std::stoi( netParams[0] )] = 1;
				//strcallback = "/NodeList/" + netParams[0] + "/ApplicationList/" + "*/ReceivedInterest";
				//Config::ConnectWithoutContext( strcallback, MakeCallback( &ReceivedInterestCallbackCom ) );

				// Setup node to originate prefixes for dynamic routing
				ndnGlobalRoutingHelper.AddOrigin( beNamePrefix, nodes.Get( std::stoi( netParams[0] ) ) );

				char temp[10];
				sprintf( temp, "%lf", 0.0/( float( rand()%40+0 ) ) );

				consumerHelper.SetPrefix( beNamePrefix + "/phy" + netParams[1] );
				consumerHelper.SetAttribute( "Frequency", StringValue( temp ) ); //0.0002 = 5000pps
				consumerHelper.SetAttribute( "Subscription", IntegerValue( 0 ) );
				consumerHelper.SetAttribute( "PayloadSize", StringValue( "1024" ) );
				consumerHelper.SetAttribute( "RetransmitPackets", IntegerValue( 0 ) );
				consumerHelper.SetAttribute( "Offset", IntegerValue( 0 ) );
				consumerHelper.SetAttribute( "LifeTime", StringValue( "100ms" ) );
				//ApplicationContainer beApps = consumerHelper.Install( nodes.Get( std::stoi( netParams[1] ) ) );
				used[std::stoi( netParams[1] )] = 1;
				//bgdApps.Start ( Seconds ( std::stod( netParams[2] ) ) );
				//bgdApps.Stop ( Seconds ( std::stod( netParams[3] ) ) );
				flow_pair.first = stoi( netParams[0] );
				flow_pair.second = stoi( netParams[1] );
				all_flows.push_back( flow_pair );
				//strcallback = "/NodeList/" + netParams[0] + "/ApplicationList/" + "*/SentInterest";
				//Config::ConnectWithoutContext( strcallback, MakeCallback( &SentInterestCallbackPhy ) );

				flowfile << netParams[0] << " " << netParams[1] << " BE" << std::endl;

				beNodeCount += 1;

			} else {
				//std::cout << "reading something else " << strLine << std::endl;
			}	
		} // end while
	} else {
		std::cout << "Cannot open configuration file!!!" << std::endl;
		exit( 1 );
	}

	configFile.close();
	std::cout<<"All done\n";

	// Define the routing strategies per prefix
	ndn::StrategyChoiceHelper::InstallAll( "/power/typeI", "/localhost/nfd/strategy/multicast" );
	ndn::StrategyChoiceHelper::InstallAll( "/power/typeII/data", "/localhost/nfd/strategy/multicast" );
	ndn::StrategyChoiceHelper::InstallAll( "/power/be", "/localhost/nfd/strategy/best-route" );
	//ndn::StrategyChoiceHelper::InstallAll( "/", "/localhost/nfd/strategy/qos" );

	ndn::AppHelper tokenHelper( "ns3::ndn::TokenBucket" );
	tokenHelper.SetAttribute( "FillRate1", StringValue( "125" ) );	// 10 interests a second
	tokenHelper.SetAttribute( "Capacity1", StringValue( "50" ) );	// 10 interests a second
	tokenHelper.SetAttribute( "FillRate2", StringValue( "100" ) );	// 10 interests a second
	tokenHelper.SetAttribute( "Capacity2", StringValue( "50" ) );	// 10 interests a second
	tokenHelper.SetAttribute( "FillRate3", StringValue( "50" ) );	// 10 interests a second
	tokenHelper.SetAttribute( "Capacity3", StringValue( "50" ) );	// 10 interests a second

	for ( int i=0; i<( int )nodes.size(); i++ ) {

		if ( used[i] == 0 ) {

			ndn::StrategyChoiceHelper::Install( nodes.Get( i ),"power/typeI", "/localhost/nfd/strategy/qos" );
			ndn::StrategyChoiceHelper::Install( nodes.Get( i ),"power/typeII/data", "/localhost/nfd/strategy/qos" );
			ndn::StrategyChoiceHelper::Install( nodes.Get( i ),"power/be", "/localhost/nfd/strategy/qos" );
			ndn::StrategyChoiceHelper::Install( nodes.Get( i ),"/", "/localhost/nfd/strategy/qos" );

			tokenHelper.Install( nodes.Get( i ) );
		}
	}

	// Populate routing table for nodes
	//ndn::GlobalRoutingHelper::CalculateRoutesg;
	ndn::GlobalRoutingHelper::CalculateAllPossibleRoutes();
	sync.beginSync();

	// Open trace file for writing
	char trace[100];
	sprintf( trace, "ndn-case1-run%d.csv", run );

	tracefile.open( trace, std::ios::out );
	tracefile << "nodeid, event, name, payloadsize, time" << std::endl;

	for ( int i=0; i < ( int )nodes.size(); i++ ) {

		if ( usedS[i] == 1 ) {

			strcallback = "/NodeList/" + std::to_string( i ) + "/ApplicationList/" + "*/SentInterest";
			Config::ConnectWithoutContext( strcallback, MakeCallback( &SentInterestCallbackPhy ) );
			strcallback = "/NodeList/" + std::to_string( i ) + "/ApplicationList/" + "*/ReceivedInterest";
			Config::ConnectWithoutContext( strcallback, MakeCallback( &ReceivedInterestCallbackPhy ) );

		}
	}

	for ( int i=0; i<( int )nodes.size(); i++ ) {

		if ( usedR[i] == 1 ) {
			strcallback = "/NodeList/" + std::to_string( i ) + "/ApplicationList/" + "*/ReceivedInterest";
			Config::ConnectWithoutContext( strcallback, MakeCallback( &ReceivedInterestCallbackCom ) );
		}
	}

	clock_t start, end;
	double cpu_time_used;
	start = clock();

	// Run simulation for 11 time steps
	Simulator::Stop( Seconds( 220.0 ) );
	Simulator::Run();
	sync.sendSync();
	Simulator::Destroy();
	end = clock();
	cpu_time_used = ( ( double ) ( end - start ) ) / CLOCKS_PER_SEC;
	std::cout<<"The simulation took "<<cpu_time_used<<std::endl;

	return 0;
}


// Check if source/destination flow is permitted in config file ( one-to-many, many-to-one flows )
bool 
FlowPermitted ( int dstnode, int srcnode ) {

	for ( int i=0; i<( int )all_flows.size(); i++ ) {

		if ( ( all_flows[i].first == dstnode ) && ( all_flows[i].second == srcnode ) ) {

			// Flow found
			return true;
		}
	}
	return false;
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
	//pos = remstr.find( "/" );
	//remstr = remstr.substr( pos+1,string::npos );

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

	for( std::string str; isstr >> str;  ) {

		result.push_back( str );
	}

	return result;
}


// Store unique PMU IDs to prevent repeated installation of app
bool 
IsPMUAppInstalled( std::string PMUID ) {

	for ( int i=0; i<( int )uniquePMUs.size(); i++ ) {

		if ( PMUID.compare( uniquePMUs[i] ) == 0 ) {

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

		if ( LCID.compare( uniqueLCs[i] ) == 0 ) {

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

		if ( WACID.compare( uniqueWACs[i] ) == 0 ) {

			return true;
		}
	}

	uniqueWACs.push_back( WACID );

	return false;
}


// Define callbacks for writing to tracefile
void 
SentInterestCallbackPhy( uint32_t nodeid, shared_ptr<const ndn::Interest> interest ) {

	if ( interest->getSubscription() == 1 || interest->getSubscription( ) == 2 ) {

		//Do not log subscription interests from phy nodes
	} else {

		tracefile << nodeid << ", sent, " << interest->getName() << ", " << interest->getPayloadLength( ) << ", " << std::fixed << setprecision( 9 )
			<< ( Simulator::Now().GetNanoSeconds( ) )/1000000000.0 << std::endl;
	}
}

void
ReceivedInterestCallbackPhy( uint32_t nodeid, shared_ptr<const ndn::Interest> interest ) {

	if ( interest->getName( ).getSubName( -3,1 ).toUri( ).substr( 1 ) == "data" ) {

		if ( DEBUG) std::cout<<"We got it "<< interest->getName( ).toUri()<<std::endl;

		// Do not log subscription interests received at com nodes
		if ( interest->getName().getSubName( 1,1 ).toUri( ) == "/typeI" ) {

			std::vector<uint8_t> payloadVector( &interest->getPayload()[0], &interest->getPayload()[interest->getPayloadLength()] );
			std::string payload( payloadVector.begin(), payloadVector.end() );
			std::string str = /*std::to_string( nodeid )*/std::to_string( interest->getPayloadLength() ) + " " + interest->getName( ).getSubName( -2,1 ).toUri( ).substr( 1 ) + " " + std::to_string( ( Simulator::Now( ).GetSeconds( ) ) ) + " OpenDSS "+payload ;

			sync.aggDER( payload, nodeid,  interest->getName().getSubName( -2,1 ).toUri().substr( 1 ), interest->getName().getSubName( 3,1 ).toUri().substr( 1 ) );
			if ( DEBUG) std::cout << "Received: " << str <<std::endl;
		}

		tracefile << nodeid << ", recv, " << interest->getName() << ", " << interest->getPayloadLength( ) << ", " << std::fixed << setprecision( 9 ) 
			<< ( Simulator::Now().GetNanoSeconds( ) )/1000000000.0 << std::endl;

	} else {

		std::stringstream sstr;
		std::string iname;
		//std::cout<<Simulator::Nowg.GetSeconds( )<<std::endl;

		if ( interest->getName().getSubName( 1,1 ).toUri( ) == "/typeI" ) {

			sstr << interest->getName().getSubName( 1 );
			sstr >> iname;
			interest->getPayload();

			std::vector<uint8_t> payloadVector( &interest->getPayload()[0], &interest->getPayload()[interest->getPayloadLength()] );
			std::string payload( payloadVector.begin(), payloadVector.end() );
			std::string str = std::to_string( interest->getPayloadLength() ) + " " + interest->getName( ).getSubName( 3,1 ).toUri( ).substr( 1 ) + " " + std::to_string( ( Simulator::Now( ).GetSeconds( ) ) ) + " RedisPV " + payload;

			std::cout<< "Received Setpoint: " << interest->getName() << "   " <<payload <<std::endl;
			bool lead = sync.sendDirect( payload, nodeid,  interest->getName().getSubName( 3,1 ).toUri().substr( 1 ) );

			if( !lead ) {
				sync.addArrivedPackets( str );
			}

			//std::cout << "Check string: " << str << std::endl;

		} else  {

			sstr << interest->getName();
			sstr >> iname;
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

	if ( interest->getSubscription() == 1 || interest->getSubscription( ) == 2 ) {

		// Do not log subscription interests received at com nodes
	} else {

		std::stringstream sstr;
		std::string iname;
		//std::cout<<Simulator::Nowg.GetSeconds( )<<std::endl;	

		if ( interest->getName().getSubName( 1,1 ).toUri( ) == "/typeI" ) {

			sstr << interest->getName().getSubName( 1 );
			sstr >> iname;

			std::vector<uint8_t> payloadVector( &interest->getPayload()[0], &interest->getPayload()[interest->getPayloadLength()] );
			std::string payload( payloadVector.begin(), payloadVector.end() );
			std::string str = /*std::to_string( nodeid )*/std::to_string( interest->getPayloadLength() ) + " " + interest->getName( ).getSubName( 3,1 ).toUri( ).substr( 1 ) + " " + std::to_string( ( Simulator::Now( ).GetSeconds( ) ) ) + " OpenDSS "+payload;

			sync.addArrivedPackets( str );
			if ( DEBUG) std::cout<< "Received: " << str <<std::endl;

		} else  {

			sstr << interest->getName();
			sstr >> iname;
		}

		//Only log flows that are permitted in config file ( eliminates redundant interests received from multiple interfaces and to other nodes )
		//if ( FlowPermitted( ( int )nodeid, ( int )GetSourceNodeID( iname ) ) == true ) {
		tracefile << nodeid << ", recv, " << interest->getName() << ", " << interest->getPayloadLength( ) << ", " << std::fixed << setprecision( 9 )
			<< ( Simulator::Now().GetNanoSeconds( ) )/1000000000.0 << std::endl;
		//}
	}
}


void 
GetComAggEdges() {

}


bool 
NodeInComm( int nodeid ) {

	for ( int i=0; i<( int )com_nodes.size(); i++ ) {

		if ( com_nodes[i] == nodeid ) {

			return true;
		}
	}

	return false;
}


bool 
NodeInAgg( int nodeid ) {

	for ( int i=0; i<( int )agg_nodes.size(); i++ ) {

		if ( agg_nodes[i] == nodeid ) {

			return true;
		}
	}
	return false;
}


} // namespace ns3



int
main( int argc, char* argv[] )
{
	return ns3::main( argc, argv );
}
