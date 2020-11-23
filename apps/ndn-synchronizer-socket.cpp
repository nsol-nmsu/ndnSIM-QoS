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
#include "ndn-synchronizer-socket.hpp"
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

#define SERVER_PORT htons( 5005 )
#define CONNECTIONS 1
#define DEBUG 0


namespace ns3 {
namespace ndn {

using json = nlohmann::json;

// On object construction create sockets and set references
SyncSocket::SyncSocket() {

  parOpen.setRefs( &nameMap, &mapDER );
  parRedis.setRefs( &nameMap, &mapDER );

  std::cout<<"Start\n";

  // Initialize socket
  sockaddr_in serverAddr;
  server_socket=socket( AF_INET, SOCK_STREAM, 0 );
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = SERVER_PORT;
  serverAddr.sin_addr.s_addr = INADDR_ANY;

  // Bind the socket
  ret = bind( server_socket, ( struct sockaddr* )&serverAddr, sizeof( struct sockaddr ) );
  if ( ret < 0 ) {
    cout << "Bind error" << endl;
  }

  // Listen for connections
  ret = listen( server_socket, CONNECTIONS );
  if ( ret < 0 ) {
    cout << "Bind error" << endl;
  }

  sockaddr_in clientAddr;
  socklen_t sin_size = sizeof( struct sockaddr_in );

  // Accept a connection
  client_socket[0] = accept( server_socket, ( struct sockaddr* )&clientAddr, &sin_size );
  client_socket[1] = accept( server_socket, ( struct sockaddr* )&clientAddr, &sin_size );
  std::string output;
  output.resize( 8 );

  // Client 1
  ret = read( client_socket[0], &output[0], 8-1 );

  if ( ret > 0 ) {
    cout << "Server received from 1:  " << output << endl;

    // Set client references based on order of connections
    if( output[0] == 'O' ){
      OpenDSS = 0;
      RedisPv = 1;
    } else {
      OpenDSS = 1;
      RedisPv = 0;
    }
  }

  // Client 2
  ret = read( client_socket[1], &output[0], BUFFER_SIZE-1 );
  if ( ret > 0 ) {
    cout << "Server received from 2:  " << output << endl;
  }
}


// Add a string to list of packets that arrived at ReDis-PV and follower DERs
// Input: Str; string with information on the arriving interest.
void
SyncSocket::addArrivedPackets( std::string str ) {

  std::vector<std::string> updateInfo = SplitString( str, 3 );
  json::iterator it = rjf.begin();

  std::cout << str << std::endl;

  // Check if the sending packet is a lead DER. If not update running measurment json.
  if ( LeadDERs[updateInfo[1]] == false ){
    std::cout<<"\nReDis-PV node ( ns3 ) received measurement data from non-PV devices\n";

    while ( it !=  rjf.end() ) {

      if ( it.key() != "Time" && it.value().find( updateInfo[1] ) != it.value().end() ) {

        it.value()[updateInfo[1]] = njf[it.key()][updateInfo[1]];
        it = rjf.end();
      } else {
        it++;
      }
    }

  } else if ( updateInfo[3] != "RedisPV" ) {
    std::cout<<"\nReDis-PV node ( ns3 ) received measurement data from PV Lead DERs\n";
  }

  // Push packet information into vector for later processing.
  arrivedPackets.push_back( str );
}


// Process packets with set point data, that arrive at a follwer DER or a lead DER and injectnew interest as needed.
// Input: Send_json; the data we will be sending. Src; the source node from which interests	will be sent.
// Device Name; the name of the reciving interest.
bool
SyncSocket::sendDirect( std::string send_json, int src, std::string deviceName ) {

  json counter = json::parse( send_json );
  json::iterator it = counter.begin();
  int elements = 0;
  bool lead = true;

  // Check if receiving packet is a follwer DER, if it is, update info for node.
  // If it is a lead DER count the number of followers it has.
  while ( it !=  counter.end() ) {

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

    elements++;
    it++;
  }

  // If node is lead DER, update tables for node and send data to OpenDSS.
  if ( lead ) {
    mapDER.erase( deviceName );
    mapDER.erase( "PV" + deviceName.substr( 4 ) );

    senders[nameMap[deviceName]]->setAsLead( deviceName );
    leads--;

    std::cout << "Lead DER " << deviceName << " set with " << elements-1 << " follower( s )\n";

    sendData( send_json, OpenDSS );

    std::cout << "Sending clustering information for Lead DER " << deviceName << " to OpenDSS" << std::endl;
    if ( DEBUG ) std::cout << "Leads left " << leads << std::endl;
  }

  // If the lead DER has at least one follower wait for response from OpenDSS.
  if ( elements > 1 && lead ) {
    std::string data = receiveData( OpenDSS );

    std::cout << "Follower DER  information ( set points ) received from Lead DER " << deviceName << " ( OpenDSS ), size: " << data.size() << std::endl;
    if ( DEBUG ) std::cout << data << std::endl;

    json follower = json::parse( data );
    processLeadJson( follower, src );
  }

  // Inform caller if the node was a lead DER or not
  return lead;
}


// Send infromation gathered over span of the timestep to the co-simulators.
void
SyncSocket::sendSync() {

  // If this is time step zero, send initialization messages.
  if ( Simulator::Now().GetSeconds() == 0 ) {
    ret = write( client_socket[OpenDSS], "ndnSIM Simulation Started",strlen( "ndnSIM Simulation Started" ) );
    ret = write( client_socket[RedisPv], "Hello, Pv",strlen( "Hello, Pv" ) );

    if ( ret < 0 ) {
      cout << "Write failed" << endl;
    }

    return;
  }

  json j;
  json openSend;

  // Process all the saved strings with packet arrival information and send any relevent data.
  while ( !arrivedPackets.empty() ) {
    std::vector<std::string> SendInfo = SplitString( arrivedPackets.back() , 0 );

    // If data is from ReDis-PV, add to json string for OpenDSS.
    if ( SendInfo[3] == "RedisPV" ) {
      openSend[SendInfo[1]] = SendInfo[4];
    } else if ( LeadDERs[SendInfo[1]] == true ) {
      // Update running json file with data that arrived at ReDis-PV from OpenDSS.
      if ( DEBUG ) std::cout << "............Updating................\n";

      json leadJson = json::parse( SendInfo[4] );
      json::iterator itLead = leadJson.begin();

      while ( itLead != leadJson.end() ) {

        if ( rjf["Storage"].find( itLead.key() ) != rjf["Storage"].end() ) {
          rjf["Storage"][itLead.key()] = njf["Storage"][itLead.key()];

          if ( DEBUG ) std::cout << rjf["Storage"][itLead.key()] << "\n...............Storage................\n";
        }

        if ( rjf["PVSystem"].find( itLead.key() ) != rjf["PVSystem"].end() ) {
          rjf["PVSystem"][itLead.key()] = njf["PVSystem"][itLead.key()];

          if ( DEBUG ) std::cout << rjf["PVSystem"][itLead.key()] << "\n..............PVSystem.................\n";
        }

        itLead++;
      }
    }

    j[SendInfo[1]]["ArrivalTime"] = SendInfo[2];
    j[SendInfo[1]]["PacketSize"] = SendInfo[0];

    arrivedPackets.pop_back();
  }

  // Send updated json files to OpenDss and ReDis-PV.
  std::string send_json = rjf.dump();

  sendData( send_json, RedisPv );
  std::cout << "Sending measurement.json to Redis-PV" << std::endl;

  send_json = openSend.dump();

  sendData( send_json, OpenDSS );
  std::cout << "Sending arrived Following DER ( at ns3 ) information to OpenDSS" << std::endl;
}

// Implmentation of sending data over cpp socket.
// Input; Data; the data to be sent. Socket; the socket we will be sending over.
void
SyncSocket::sendData( std::string data, int socket ) {

  json packet;
  int i;
  int n = ( data.size()/750 ) + 1;
  std::string chunks[n];

  // Find nummber of chunks.
  for ( i = 0; i<n; i++ ) {
    chunks[i] = data.substr( i*750, 750 );
  }

  // Set first key in packet dictionary - total size of one timestep data.
  packet["size"] =  data.size();
  i = 0;

  // Create packets out of chunks and send over socket.
  while ( i < n ) {

    if ( i < n-1 ) {
      packet["finished"] = 0;
    } else {
      packet["finished"] = 1;
    }

    packet["payload"]  = chunks[i];
    packet["payloadsize"] = chunks[i].size();

    //std::cout << " Payload size : " << chunks[i].size() << std::endl;

    std::string zeroStr = "";
    packet["0"] = zeroStr;
    std::string pj = packet.dump();
    int zeros = 1023 - pj.size();

    for ( int z = 0; z < zeros; z++ ) {
      zeroStr += "0";
    }

    packet["0"] = zeroStr;
    pj = packet.dump();

    //std::cout << " PJ size : " << pj.size() << std::endl;

    i += 1;
    ret = write( client_socket[socket],&pj[0],pj.size() );

    if ( ret < 0 ) {
      cout << "Write failed" << endl;
    }

  }
}


// Receive data from co-simulators.
void
SyncSocket::receiveSync() {

  std::string data = receiveData( OpenDSS );
  njf = json::parse( data );

  std::cout << "\nMeasurement data received from OpenDSS, size: " << data.size() << " bytes" << std::endl;

  data = receiveData( RedisPv );
  parOpen.processJson( njf, &packetNames );

  std::cout << "\n" <<  data <<"\n";

  if( data != "" ) {
    json cluster_info = json::parse( data );
    parRedis.processJson( cluster_info,  &packetNames, &leads );
    std::string send = to_string( leads );
    ret = write( client_socket[OpenDSS],&send[0],send.size() );
  } else {
    std::string send = to_string( 0 );
    ret = write( client_socket[OpenDSS],&send[0],send.size() );
  }

  std::cout << "Clustering information ( set points ) received from ReDis-PV, size: " << data.size() << " bytes" << std::endl;

}

// Implmentation for receiving data packet.
// Input: Socket; the reciving socket ( for OpenDSS or ReDis-PV ).
std::string
SyncSocket::receiveData( int socket ) {

  bool done = false;
  std::string output, buff;
  output = "";
  buff.resize( BUFFER_SIZE );
  int rec = 0;
  int size = 0;

  // Read data from socket in pre-defined chunk sizes.
  while ( !done ) {

    bzero( buffer, BUFFER_SIZE );

    // Receive a message from a client.
    ret = read( client_socket[socket], &buff[0], BUFFER_SIZE-1 );

    if ( ret > 0 && ( ret != 1 || buff[0] != ' ' ) ) {
      //cout << "Confirmation code  " << ret << endl;
      //cout << "Server received:  " << output << endl;

      while ( ret < 1023 ) {
        int pret = read( client_socket[socket], &buff[ret], BUFFER_SIZE-( ret +1 ) );
        ret += pret;
      }

      json tempj = json::parse( buff );
      size = tempj["size"];

      if( tempj["finished"] == 1 ) {
        done = true;
      }

      output +=  tempj["payload"];
      int temp = tempj["payloadsize"];
      rec  += temp;
    } else {
      done = true;
    }

  }

  return output;
}


// Process measurment Json from OipenDSS using gloabl njf as source.
/*void
SyncSocket::processJson(){

	json::iterator it = njf.begin();

	//Loop through received Json file and create packets for sending over ndnSIM
	while ( it !=  njf.end() ) {

		int consumer;
		std::string device;
		std::string packet_insert;
		int payloadSize = 150;
		//std::cout << it.key() << "\n";

		if ( it.key() == "Transformer" || it.key() == "Capacitor" || it.key() == "PVSystem" || it.key() == "Storage" ) {

			json::iterator it1 = it.value().begin();

			while ( it1 != it.value().end() ) {

				consumer = nameMap[it1.key()];
				device = it1.key();
				json::iterator it2 = it1.value().begin();
				std::string payload = it1.value().dump();
				payloadSize = payload.size();

				while ( it2 != it1.value().end() ) {

					//std::cout << it2.key() << "\n";
					if ( it2.key() == "busdata" ) {

						//std::cout << it2.key() << " : " << it2.value() << "\n";
						//consumer = it1.value();
					}

					it2++;
				}

				if ( mapDER.find( it1.key() ) != mapDER.end() ) {

					packet_insert = "phy" + std::to_string( nameMap[mapDER[it1.key()]] ) + "/" + mapDER[it1.key()] + "/data/" + device + " " + std::to_string( consumer ) +  " " + payload;
				} else {

					packet_insert = "data/" + device + " " + std::to_string( consumer ) +  " " + payload;
				}

				packetNames.push_back( packet_insert );
				it1++;
			}
		}

		it++;
	}

}*/

//Process Json file from ReDis-PV, creating needed packets as well.
//Input: jf; the recived json file from ReDis-PV.
/*void
SyncSocket::processRPVJson( json jf ) {

	json::iterator it = jf.begin();

	while ( it !=  jf.end() ) {

		leads++;
		std::string device;
		std::string payload;
		int payloadSize;
		std::string packet_insert;
		json::iterator it1 = it.value().begin();

		while ( it1 !=  it.value().end() ) {

			if ( it1.key() == "Lead_DER" ) {
				device = "phy" + std::to_string( nameMap[it1.value()] ) + "/";
				device += it1.value();
			} else {
				payload = it1.value().dump();
				payloadSize = payload.size();
			}

			it1++;
		}
		packet_insert = device + " " + std::to_string( PVNode ) +  " " + payload;
		if ( DEBUG )   std::cout << packet_insert << std::endl;
		packetNames.push_back( packet_insert );
		it++;
	}

	std::string send = to_string( leads );
	ret = write( client_socket[OpenDSS],&send[0],send.size() );
}
*/


// Process Json file from Lead DER ( openDSS ) and create appropriate packets.
// Input: jf; the recived json file from OpenDSS. Src; the ns-3 node that will be the source of new interests.
void
SyncSocket::processLeadJson( json jf, int src ) {

  json::iterator it = jf.begin();

  while ( it !=  jf.end() ) {
    std::string device;
    std::string payload;
    int payloadSize;
    std::string packet_insert;

    device = "phy"+ std::to_string( nameMap[it.key()] ) + "/";
    device += it.key();
    device += "/Lead"+std::to_string( src );

    payload = it.value().dump();
    payloadSize = payload.size();
    packet_insert = device + " " + std::to_string( src ) +  " " + payload;

    if ( DEBUG ) std::cout << packet_insert << std::endl;

    packetNames.push_back( packet_insert );
    it++;
  }

  injectInterests( false );
}


// Fill out hash map where we link deivce names to ns-3 nodes.
// Input: Json; has information on what names go with what device.
void
SyncSocket::fillNameMap( json jf ) {

  json::iterator it = jf.begin();

  while ( it !=  jf.end() ) {
    nameMap[it.key()]=it.value();
    it++;
  }

}


// Create the first instance of our running measumernt Json.
// Input: Json; An instance of th measurment data.
void
SyncSocket::initializeJson( json jf ){

  rjf =  jf;
}

// When packet arrives at Lead DER from follower DER, update Lead payload and send new packet to ReDis-PV.
// Input: Payload; the payload of the arriving interest. follower; the source node for interest Lead;
// the node the recived the interest.
void
SyncSocket::aggDER( std::string payload, int src, std::string follower, std::string lead ) {

  std::cout << "\nLead DER " << lead << " received  measurments from Follower DER " << follower << ", size: " << payload.size() << std::endl;
  senders[nameMap[lead]]->updateLeadMeasurements( payload, follower );
  LeadDERs[lead] = true;
  LeadDERs[follower] = false;
  int consumer = nameMap[lead];

  std::string packet_insert = "data/" + lead + " " + std::to_string( consumer ) +  " " + payload;
  packetNames.push_back( packet_insert );

  injectInterests( true );
}


}
}
