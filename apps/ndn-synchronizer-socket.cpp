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

#define SERVER_PORT htons(  5005  )
#define CONNECTIONS 1


namespace ns3 {
namespace ndn {

using json = nlohmann::json;

SyncSocket::SyncSocket() {

	std::cout<<"Start\n";

	sockaddr_in serverAddr;
	server_socket=socket(  AF_INET, SOCK_STREAM, 0  );
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = SERVER_PORT;
	serverAddr.sin_addr.s_addr = INADDR_ANY;

	// Bind the socket
	ret = bind(  server_socket, (  struct sockaddr*  )&serverAddr, sizeof(  struct sockaddr  )  );
	if (  ret < 0  ) {
		cout << "Bind error" << endl;
	}

	// Listen for connections
	ret = listen(  server_socket, CONNECTIONS  );
	if (  ret < 0  ) {
		cout << "Bind error" << endl;
	}

	sockaddr_in clientAddr;
	socklen_t sin_size = sizeof(  struct sockaddr_in  );

	// Accept a connection
	client_socket = accept(  server_socket, (  struct sockaddr*  )&clientAddr, &sin_size  );
}

void
SyncSocket::addArrivedPackets( std::string str ) {

	std::vector<std::string> updateInfo = SplitString(  str  );

	json::iterator it = rjf.begin();

	while ( it !=  rjf.end() ) {

		if ( it.key() != "Time" && it.value().find( updateInfo[1] ) != it.value().end() ) {

			it.value()[updateInfo[1]] = njf[it.key()][updateInfo[1]];
			it = rjf.end();
		} else {
			it++;
		}
	}

	arrivedPackets.push_back( str );
}


void
SyncSocket::sendSync(){

	//ret = write(  client_socket, buffer, strlen(  buffer  )  );
	//ret = write(  client_socket,"Hi",strlen( "Hi" )   );

	if ( Simulator::Now().GetSeconds() == 0 ) {

		ret = write(  client_socket,"ndnSIM Simulation Started",strlen( "ndnSIM Simulation Started" )   );

		if (  ret < 0  ) {
			cout << "Write failed" << endl;
		}
		return;
	}

	json j;

	while ( !arrivedPackets.empty() ) {

		std::vector<std::string> SendInfo = SplitString(  arrivedPackets.back()  );
		//std::cout<<arrivedPackets.back()<<std::endl;
		j[SendInfo[1]]["ArrivalTime"]= SendInfo[2];
		j[SendInfo[1]]["PacketSize"]= SendInfo[0];
		arrivedPackets.pop_back();
	}

	std::string send_json = rjf.dump();
	//std::cout<<j.dump( 4 )<<std::endl;
	std::cout<<send_json.size()<<std::endl;

	json packet;
	int i;
	int n = ( send_json.size()/750 )+1;
	std::string chunks[n];

	for ( i = 0; i<n; i++ ) {
		chunks[i] = send_json.substr( i*750,750 ); 
	}

	packet["size"] =  send_json.size();	// Set first key in packet dictionary - total size of one timestep data
	i = 0;

	while ( i < n ) {

		if ( i < n-1 ) {
			packet["finished"] = 0;
		} else {
			packet["finished"] = 1;
		}

		packet["payload"]  = chunks[i];
		packet["payloadsize"] = chunks[i].size();

		//std::cout<< " Payload size : " << chunks[i].size() << std::endl;

		std::string zeroStr = "";
		packet["0"] = zeroStr;
		std::string pj = packet.dump();
		int zeros = 1023 - pj.size();

		for ( int z = 0; z < zeros; z++ ) {
			zeroStr += "0";
		}

		packet["0"] = zeroStr;
		pj = packet.dump();

		//std::cout<< " PJ size : " << pj.size() << std::endl;

		i += 1;
		ret = write(  client_socket,&pj[0],pj.size()   );

		if (  ret < 0  ) {
			cout << "Write failed" << endl;
		}
	}
}


void
SyncSocket::receiveSync(){

	bool done = false;
	std::string output, buff;
	buff = "";
	output.resize( BUFFER_SIZE );
	int rec = 0;
	int size = 0;

	while (  !done  ) { 

		bzero(  buffer, BUFFER_SIZE  );

		// Receive a message from a client
		ret = read(  client_socket, &output[0], BUFFER_SIZE-1  );

		if (  ret > 0  ) {

			cout << "Confirmation code  " << ret << endl;
			//cout << "Server received:  " << output << endl;

			while ( ret < 1023 ) {

				int pret = read(  client_socket, &output[ret], BUFFER_SIZE-( ret +1 )  );
				cout << "Building... " << pret << endl;
				ret += pret;

			}

			json tempj = json::parse( output );
			size = tempj["size"];

			if(  tempj["finished"] == 1 ) {
				done = true;
			}

			buff +=  tempj["payload"];
			int temp = tempj["payloadsize"];
			rec  += temp;
		} else {
			//done = true;
		}
	}

	njf = json::parse( buff );
	processJson();
	std::cout<<"ends\n";
}


void
SyncSocket::processJson(){

	json::iterator it = njf.begin();

	while ( it !=  njf.end() ) {

		int consumer;
		std::string device;
		std::string packet_insert;
		int payloadSize = 150;
		//std::cout<<it.key()<<"\n";

		//TODO:Add storage
		if ( it.key() == "Transformer" || it.key() == "Capacitor" || it.key() == "PVSystem" ) {

			json::iterator it1 = it.value().begin();

			while ( it1 != it.value().end() ) {

				consumer = nameMap[it1.key()];
				device = it1.key();
				json::iterator it2 = it1.value().begin();
				std::string payload = it1.value().dump();
				payloadSize = payload.size();

				while ( it2 != it1.value().end() ) {

					//td::cout<<it2.key()<<"\n";
					if ( it2.key() == "busdata" ) {

						//std::cout << it2.key() << " : " << it2.value() << "\n";
						//consumer = it1.value();
					}

					it2++;
				}

				packet_insert = device + " " +std::to_string( consumer ) +  " " + payload;
				packetNames.push_back( packet_insert );
				it1++;
			}
		}

		it++;
	}

}


void
SyncSocket::fillNameMap( json jf ){

	json::iterator it = jf.begin();

	while ( it !=  jf.end() ) {

		nameMap[it.key()]=it.value();
		it++;
	}
}


void
SyncSocket::initializeJson( json jf ){

	rjf =  jf;
}


}
}
