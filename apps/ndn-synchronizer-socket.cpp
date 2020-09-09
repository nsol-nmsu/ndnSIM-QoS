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


namespace ns3 {
namespace ndn {

using json = nlohmann::json;

SyncSocket::SyncSocket() {

	std::cout<<"Start\n";

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

		cout << "Confirmation code  " << ret << endl;
		cout << "Server received from 1:  " << output << endl;

		if(output[0] == 'O'){

			cout << "First" << endl;
			OpenDSS = 0;
			RedisPv = 1;
		} else {
			cout << "Second" << endl;
			OpenDSS = 1;
			RedisPv = 0;
		}
	}

	// Client 2
	//client_socket[1] = accept( server_socket, ( struct sockaddr* )&clientAddr, &sin_size );
	ret = read( client_socket[1], &output[0], BUFFER_SIZE-1 );
	if ( ret > 0 ) {
		cout << "Confirmation code  " << ret << endl;
		cout << "Server received from 2:  " << output << endl;
	}
}


void
SyncSocket::addArrivedPackets( std::string str ) {

	std::vector<std::string> updateInfo = SplitString( str ,2 );

	json::iterator it = rjf.begin();

	while ( LeadDERs[updateInfo[1]] == false && it !=  rjf.end() ) {

		if ( it.key() != "Time" && it.value().find( updateInfo[1] ) != it.value().end() ) {

			it.value()[updateInfo[1]] = njf[it.key()][updateInfo[1]];
			it = rjf.end();
		} else {
			it++;
		}
	}

	arrivedPackets.push_back( str );
}


bool
SyncSocket::sendDirect( std::string send_json, int src, std::string deviceName ) {

	json counter = json::parse( send_json );
	json::iterator it = counter.begin();
	int elements = 0;
	bool lead = true;

	while ( it !=  counter.end() ) {

		if ( it.key() == "P" ) {

			lead = false;
			senders[nameMap[deviceName]]->resetLead();
		}

		if ( it.key() == "Lead_DER" ) {

			mapDER[deviceName] = it.value();
			mapDER["PV"+deviceName.substr(4)] =  it.value();
		}

		elements++;
		it++;
	}

	if ( lead ) {

		mapDER.erase( deviceName );
		mapDER.erase( "PV" + deviceName.substr( 4 ) );
		senders[nameMap[deviceName]]->setAsLead( deviceName );
		leads--;

		if ( elements > 1 ) {
			std::cout << elements << " Got some Lead data\n";
		} else {
			std::cout << elements << " Got some non-lead data\n";
		}

		sendData( send_json, OpenDSS );
		std::cout << "Leads left " << leads << std::endl;
	}

	if ( elements > 1 && lead ) {

		std::string data = receiveData( OpenDSS );
		std::cout << data << std::endl;
		json follower = json::parse( data );
		processLeadJson( follower, src );
	}

	return lead;
}


void
SyncSocket::sendSync() {

	if ( Simulator::Now().GetSeconds() == 0 ) {

		ret = write( client_socket[OpenDSS],"ndnSIM Simulation Started",strlen( "ndnSIM Simulation Started" ) );
		ret = write( client_socket[RedisPv],"Hello, Pv",strlen( "Hello, Pv" ) );

		if ( ret < 0 ) {
			cout << "Write failed" << endl;
		}

		return;
	}

	json j;
	json openSend;

	while ( !arrivedPackets.empty() ) {

		std::vector<std::string> SendInfo = SplitString( arrivedPackets.back() , 0 );

		if ( SendInfo[3] == "RedisPV" ) {

			openSend[SendInfo[1]] = SendInfo[4];

		} else if ( LeadDERs[SendInfo[1]] == true ) {

			std::cout<<"............Updating................\n";
			json leadJson = json::parse( SendInfo[4] );
			json::iterator itLead = leadJson.begin();

			while ( itLead != leadJson.end() ) {

				if ( rjf["Storage"].find( itLead.key() ) != rjf["Storage"].end() ) {

					rjf["Storage"][itLead.key()] = njf["Storage"][itLead.key()];
					std::cout << rjf["Storage"][itLead.key()] << "\n...............Storage................\n";

				}

				if ( rjf["PVSystem"].find( itLead.key() ) != rjf["PVSystem"].end() ) {

					rjf["PVSystem"][itLead.key()] = njf["PVSystem"][itLead.key()];
					std::cout << rjf["PVSystem"][itLead.key()] << "\n..............PVSystem.................\n";

				}

				itLead++;
			}
		}

		j[SendInfo[1]]["ArrivalTime"] = SendInfo[2];
		j[SendInfo[1]]["PacketSize"] = SendInfo[0];
		arrivedPackets.pop_back();
	}

	std::string send_json = rjf.dump();
	//std::cout << j.dump( 4 ) << std::endl;
	std::cout << send_json.size() << std::endl;

	sendData( send_json, RedisPv );

	send_json = openSend.dump();
	sendData( send_json, OpenDSS );
	std::cout << "Lets check\n";
	std::cout << openSend.dump( 4 ) << std::endl;
}

void
SyncSocket::sendData( std::string data, int socket ) {

	json packet;
	int i;
	int n = ( data.size()/750 ) + 1;
	std::string chunks[n];

	for ( i = 0; i<n; i++ ) {
		chunks[i] = data.substr( i*750, 750 );
	}

	packet["size"] =  data.size();     // Set first key in packet dictionary - total size of one timestep data
	i = 0;

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
		//ret = write( client_socket[OpenDSS],&pj[0],pj.size()  );
		ret = write( client_socket[socket],&pj[0],pj.size() );
		if ( ret < 0 ) {
			cout << "Write failed" << endl;
		}
	}
}


void
SyncSocket::receiveSync() {

	std::string data = receiveData( OpenDSS );
	njf = json::parse( data );

	std::cout << "OpenDSS json recieved with size " << njf.size() << std::endl;

	data = receiveData( RedisPv );
	processJson();
	json test = json::parse( data );
	processRPVJson( test );

	std::cout << "RedisPv json recieved with size " << test.size() << std::endl;
}

std::string
SyncSocket::receiveData( int socket ) {

	bool done = false;
	std::string output, buff;
	output = "";
	buff.resize( BUFFER_SIZE );
	int rec = 0;
	int size = 0;

	while ( !done ) {

		bzero( buffer, BUFFER_SIZE );

		// Receive a message from a client
		ret = read( client_socket[socket], &buff[0], BUFFER_SIZE-1 );
		if ( ret > 0 ) {

			//cout << "Confirmation code  " << ret << endl;
			//cout << "Server received:  " << output << endl;
			while ( ret < 1023 ) {

				int pret = read( client_socket[socket], &buff[ret], BUFFER_SIZE-( ret +1 ) );
				//cout << "Building... " << pret << endl;
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
		}
	}

	return output;
}


void
SyncSocket::processJson(){

	json::iterator it = njf.begin();

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

}

void
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
		std::cout << packet_insert << std::endl;
		packetNames.push_back( packet_insert );
		it++;
	}

	std::string send = to_string(leads);
	ret = write( client_socket[OpenDSS],&send[0],send.size() );
}

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
		payload = it.value().dump();
		payloadSize = payload.size();

		packet_insert = device + " " + std::to_string( src ) +  " " + payload;
		std::cout << packet_insert << std::endl;
		packetNames.push_back( packet_insert );
		it++;
	}

	injectInterests( false );
}

void
SyncSocket::fillNameMap( json jf ) {

	json::iterator it = jf.begin();

	while ( it !=  jf.end() ) {

		nameMap[it.key()]=it.value();

		if ( it.key().substr( 0, 4 ) == "BESS" ) {
			std::cout << it.key().substr( 0, 4 ) << " should have ten\n";
		}

		it++;
	}
}


void
SyncSocket::initializeJson( json jf ){

	rjf =  jf;
}


void
SyncSocket::setEntryDER( std::string der, std::string lead ) {
}


void
SyncSocket::aggDER( std::string payload, int src, std::string follower, std::string lead ) {

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
