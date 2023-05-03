/*
 * Copyright ( C ) 2020 New Mexico State University
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

#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include "ndn-synchronizer-Loss.hpp"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/integer.h"
#include "ns3/double.h"

using namespace std;

#define DEBUG 0
#define LOSS 0
#define LOSS1 704
#define LOSS2 444
#define LOSS3 101

namespace ns3 {
namespace ndn {

using json = nlohmann::json;

// On object construction create sockets and set references
SyncLoss::SyncLoss() {

  parOpen.setRefs( &nameMap, &mapDER );
  parRedis.setRefs( &nameMap, &mapDER );

  socket.start();
}


void
SyncLoss::syncEvent(){
   Synchronizer::syncEvent();
   injectInterests( false, false );
}


// Add a string to list of packets that arrived at ReDis-PV and follower DERs
// Input: Str; string with information on the arriving interest.
void
SyncLoss::addArrivedPackets( std::string str ) {

  std::vector<std::string> updateInfo = SplitString( str, 3 );
  json::iterator it = rjf.begin();

  // Check if the sending packet is a lead DER. If not update running measurment json.
  if ( LeadDERs[updateInfo[1]] == false &&  updateInfo[3] != "RedisPV" ){
    //std::cout<<"\nReDis-PV node ( ns3 ) received measurement data from non-PV devices\n";
    if(dropMap[nameMap[updateInfo[1]]]){
	    return;
    }


    while ( it !=  rjf.end() ) {

      if ( it.key() != "Time" && it.value().find( updateInfo[1] ) != it.value().end()) {

	//&& it.value()[updateInfo[1]].begin() != it.value()[updateInfo[1]].end()) {
        it.value()[updateInfo[1]] = njf[it.key()][updateInfo[1]];
	//rSend[it.key()][updateInfo[1]] = njf[it.key()][updateInfo[1]];
		
        it = rjf.end();

      } else {
        it++;
      }
    }

  } else if ( updateInfo[3] != "RedisPV" ) {
       //std::cout << "Measurment recived from\n " << str << std::endl;

    //std::cout<<"\nReDis-PV node ( ns3 ) received measurement data from PV Lead DERs\n";
  }

  // Push packet information into vector for later processing.
  arrivedPackets.push_back( str );
}


// Process packets with set point data, that arrive at a follwer DER or a lead DER and injectnew interest as needed.
// Input: Send_json; the data we will be sending. Src; the source node from which interests	will be sent.
// Device Name; the name of the reciving interest.
bool
SyncLoss::sendDirect( std::string send_json, int src, std::string deviceName ) {


  //if (dropMap[nameMap[deviceName]]) return true;  

  json counter = json::parse( send_json );
  json::iterator it = counter.begin();
  int elements = 0;
  bool lead = true;
  std::string data = "";
  // Check if receiving packet is a follwer DER, if it is, update info for node.
  // If it is a lead DER count the number of followers it has.
  while ( it !=  counter.end() ) {
    //std::cout<<it.key()<<std::endl;

    // If we find measurment Data that means this is a follower DER
    if ( it.key() == "P" ) {
      lead = false;
      senders[nameMap[deviceName]]->resetLead();
    }

    // Update the lead DER information for this node
    if ( it.key() == "Lead_DER" ) {
      mapDER[deviceName] = it.value();
      mapDER["PV"+deviceName.substr( 4 )] =  it.value();
      std::cout << "Follower DER " << deviceName << " set with Lead DER " << it.value() << "\n";
    }

    if(it.key() == "BESS_Setpoints"){
       data =  it.value().dump();        
    }

    elements++;
    it++;
  }

  // If node is lead DER, update tables for node and send data to OpenDSS.
  if ( lead ) {
    mapDER.erase( deviceName );
    mapDER.erase( "PV" + deviceName.substr( 4 ) );
    std::vector<string> devices;
    devices.push_back(deviceName);
    devices.push_back("PV" + deviceName.substr( 4 ));
    senders[nameMap[deviceName]]->setAsLead( devices );

    //if ( DEBUG ) std::cout << "Leads left " << leads << std::endl;
  }

  // If the lead DER has at least one follower wait for response from OpenDSS.
  if ( lead ) {
    //std::string data = receiveData( OpenDSS );

    //std::cout << "Lead DER " << deviceName << " set\n";
    //std::cout << "Follower DER  information ( set points ) received from Lead DER " << deviceName << " ( OpenDSS ), size: " << data.size() << std::endl;
    if ( DEBUG ) std::cout << data << std::endl;

    //json follower = json::parse( counter );
   //std::cout<<counter.dump()<<std::endl;
    processLeadJson( counter, src, deviceName );
   //std::cout<<"why\n";
  }

  // Inform caller if the node was a lead DER or not
  return lead;
}


// Send infromation gathered over span of the timestep to the co-simulators.
void
SyncLoss::sendSync() {

  // If this is time step zero, send initialization messages.
  if ( Simulator::Now().GetSeconds() == 0 ) {
    //ret = write( client_socket[OpenDSS], "ndnSIM Simulation Started",strlen( "ndnSIM Simulation Started" ) );
    //ret = write( client_socket[RedisPv], "Hello, Pv",strlen( "Hello, Pv" ) );

    //if ( ret < 0 ) {
    //  cout << "Write failed" << endl;
    //}

    return;
  }

  json j;
  json openSend;
  if(OSendTemp.dump() != "{}")
	  openSend["Switchpositions"] = OSendTemp["Switchpositions"];

  // Process all the saved strings with packet arrival information and send any relevent data.
  while ( !arrivedPackets.empty() ) {
    std::vector<std::string> SendInfo = SplitString( arrivedPackets.back() , 0 );

    // If data is from ReDis-PV, add to json string for OpenDSS.
    //std::cout<<arrivedPackets.back()<<std::endl;
    if ( SendInfo[3] == "RedisPV" ) {
      openSend["setpoints"][SendInfo[1]] = SendInfo[4];
    } 
    else if ( LeadDERs[SendInfo[1]] == true  && SendInfo[4] != "") {

      if(dropMap[nameMap[SendInfo[1]]]){
         arrivedPackets.pop_back();
	 continue;
      }
	    

      // Update running json file with data that arrived at ReDis-PV from OpenDSS.
      if ( DEBUG ) {
         std::cout << "............Updating................\n";
	 std::cout<<"Lead Json: "<<SendInfo[4]<<"\n"<<arrivedPackets.back()<<std::endl;
      }

      json leadJson = json::parse( SendInfo[4] );
      //std::cout << leadJson.dump(2) << std::endl;
      json::iterator itLead = leadJson.begin();

      while ( itLead != leadJson.end() ) {

        if ( rjf["Storage"].find( itLead.key() ) != rjf["Storage"].end() ) {
          rjf["Storage"][itLead.key()] = njf["Storage"][itLead.key()];
	  //rSend["Storage"][itLead.key()] = njf["Storage"][itLead.key()];

          if ( DEBUG ) std::cout << rjf["Storage"][itLead.key()] << "\n...............Storage................\n";
        }

	else if ( rjf["PVSystem"].find( itLead.key() ) != rjf["PVSystem"].end() ) {
          rjf["PVSystem"][itLead.key()] = njf["PVSystem"][itLead.key()];
	  //rSend["PVSystem"][itLead.key()] = njf["PVSystem"][itLead.key()];

          if ( DEBUG ) std::cout << rjf["PVSystem"][itLead.key()] << "\n..............PVSystem.................\n";
        }

        itLead++;
      }
    }

    j[SendInfo[1]]["ArrivalTime"] = SendInfo[2];
    j[SendInfo[1]]["PacketSize"] = SendInfo[0];

    arrivedPackets.pop_back();
  }

  std::string send_json;
  // Send updated json files to OpenDss and ReDis-PV.
  //if ( Simulator::Now().GetSeconds() <= 5 )
  send_json = rjf.dump();
  //else 
  //   send_json = rSend.dump();
  
  /*json::iterator it = rSend.begin();
  while ( it !=  rSend.end() ) {
     json::iterator it1 = it.value().begin();
     while ( it1 !=  it.value().end() ) {
        it1.value().clear();
	it1++;
     }
     it++;
  }*/


  std::cout << "Sending measurement.json to Redis-PV" << std::endl;
  //std::cout << "\nsendSync()============================================ Sending to ReDis-PV: \n"<<send_json<< std::endl;
  socket.sendData( send_json, 'R' );

  send_json = openSend.dump();

  std::cout << "Sending arrived DER ( at ns3 ) information to OpenDSS" << std::endl;
  //std::cout << "\nsendSync()============================================ Sending to OpenDSS: \n"<<openSend<< std::endl;
  socket.sendData( send_json, 'O' );
}

// Receive data from co-simulators.
void
SyncLoss::receiveSync() {

   if (endDrop <= Simulator::Now().GetSeconds()){
      for (auto i : nameMap) {
         dropMap[i.second] = false;
      }

      for (auto i : nameMap) {
         if((i.first.substr(0,1) == "B" || i.first.substr(0,1) == "V" || i.first.substr(0,1) == "P" ) &&
  	 	dropMap[i.second] != true && (rand()%1000) >= LOSS1){
 	    
	    dropMap[i.second] = true;
	 }
	 else if ((i.first.substr(0,1) == "P") &&
            dropMap[i.second] != true && (rand()%1000) >= LOSS2){

	    dropMap[i.second] = true;
	 }
	 else if ((rand()%1000) >= LOSS3) {
 	    dropMap[i.second] = true;
     	 }
      }
      int timSteps = 5;
      endDrop = Simulator::Now().GetSeconds() + double(timSteps);
   }


  std::string data = socket.receiveData('O' );
  //std::cout << "\nreceiveSync()============================================ Data from OpenDSS: \n"<<data.size()<< " bytes" << std::endl;

  njf = json::parse( data );

  std::cout << "\nMeasurement data received from OpenDSS, size: " << data.size() << " bytes" << std::endl;
  //std::cout <<njf<< std::endl;
  //rSend = njf;
  if ( Simulator::Now().GetSeconds() == 0 ) 
     rjf = njf;
  /*json::iterator it = rSend.begin();
  while ( it !=  rSend.end() ) {
     json::iterator it1 = it.value().begin();
     while ( it1 !=  it.value().end() ) {
        it1.value().clear();
        it1++;
     }
     it++;
  }*/

  data = socket.receiveData( 'R' );
  //std::cout << "\nreceiveSync()============================================ Data from ReDis-PV: \n"<<data<< std::endl;
  OSendTemp = json::parse( data );
  int time = parOpen.processJson( njf, &packetNames );

  if(time != 0){
     rjf["Time"] = time;
     //rSend["Time"] = time;
  }

  //std::cout << "\n" <<  data <<"\n";

  if( data != "" ) {
    json cluster_info = json::parse( data );
    parRedis.processJson( cluster_info,  &packetNames, &leads );
    //std::string send = to_string( 0 );
    //ret = write( client_socket[OpenDSS],&send[0],send.size() );
  } 

  std::cout << "Clustering information ( set points ) received from ReDis-PV, size: " << data.size() << " bytes" << std::endl;

}

// Process Json file from Lead DER ( openDSS ) and create appropriate packets.
// Input: jf; the recived json file from OpenDSS. Src; the ns-3 node that will be the source of new interests.
void
SyncLoss::processLeadJson( json jf, int src, std::string srcDevice ) {

  json::iterator it = jf.begin();

  while ( it !=  jf.end() ) {
    std::string payload;
    int payloadSize;
    payload = it.value().dump();
    payloadSize = payload.size();

    if(it.key()!= srcDevice){
      std::string device;
      std::string packet_insert;

      device = "phy"+ std::to_string( nameMap[it.key()] ) + "/";
      device += it.key();
      device += "/Lead"+std::to_string( src );

      packet_insert = device + " " + std::to_string( src ) +  " " + payload;

      if ( DEBUG ) std::cout << packet_insert << std::endl;

      packetNames.push_back( packet_insert );
    }
    else {
	
       std::string str = std::to_string( payload.length()) + " " + srcDevice + " "
   	       + std::to_string( ( Simulator::Now( ).GetSeconds( ) ) ) + " RedisPV " + payload;
       addArrivedPackets( str );
    }
    it++;
  }

  injectInterests( false, true );
}


// Fill out hash map where we link deivce names to ns-3 nodes.
// Input: Json; has information on what names go with what device.
void
SyncLoss::fillNameMap( json jf ) {

  json::iterator it = jf.begin();

  while ( it !=  jf.end() ) {
    nameMap[it.key()]=it.value();
    it++;
  }

}


// Create the first instance of our running measumernt Json.
// Input: Json; An instance of th measurment data.
void
SyncLoss::initializeJson( json jf ){

  rjf =  jf;
}

// When packet arrives at Lead DER from follower DER, update Lead payload and send new packet to ReDis-PV.
// Input: Payload; the payload of the arriving interest. follower; the source node for interest Lead;
// the node the recived the interest.
void
SyncLoss::aggDER( std::string payload, int src, std::string follower, std::string lead ) {

  if (dropMap[nameMap[follower]]) return;  

  //std::cout << "\nLead DER " << lead << " received  measurments from Follower DER " << follower << ", size: " << payload.size() << std::endl;
  senders[nameMap[lead]]->updateLeadMeasurements( payload, follower );
  LeadDERs[lead] = true;  
  LeadDERs[follower] = false;
  
  int consumer = nameMap[lead];
  std::string packet_insert = "data/" + lead + " " + std::to_string( consumer ) +  " " + payload;
  packetNames.push_back( packet_insert );

  injectInterests( true, false );
}


}
}
