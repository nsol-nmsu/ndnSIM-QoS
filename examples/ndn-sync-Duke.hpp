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

#include "apps/ndn-synchronizer-Loss.hpp"
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

// Split a line into a vector of strings
std::vector<std::string> SplitString( std::string );

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

ns3::ndn::SyncLoss sync;

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

        if ( interest->getName( ).getSubName( 2,1 ).toUri( ).substr( 0,5 ) == "/back" ) 
	{
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
	else if ( interest->getName().getSubName( -2,1 ).toUri( ) == "/attack"){
  	   //std::cout<<"Attack"<<Simulator::Now().GetSeconds( )<<std::endl;
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
   else if ( interest->getName().getSubName( -2,1 ).toUri( ) == "/attack"){
      //std::cout<<"Attack"<<Simulator::Now().GetSeconds( )<<std::endl;
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
