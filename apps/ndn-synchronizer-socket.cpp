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

// On object construction create sockets and set references
SyncSocket::SyncSocket() {

}

void
SyncSocket::start(){

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
    exit(0);
  }

  // Listen for connections
  ret = listen( server_socket, CONNECTIONS );
  if ( ret < 0 ) {
    cout << "Bind error" << endl;
  }

  sockaddr_in clientAddr;
  socklen_t sin_size = sizeof( struct sockaddr_in );

  // Accept a connection
  client_socket.push_back(accept( server_socket, ( struct sockaddr* )&clientAddr, &sin_size ));
  client_socket.push_back(accept( server_socket, ( struct sockaddr* )&clientAddr, &sin_size ));
  std::string output;
  output.resize( 8 );

  // Client 1
  ret = read( client_socket[0], &output[0], 8-1 );

  if ( ret > 0 ) {
    cout << "Server received from 1:  " << output << endl;

    // Set client references based on order of connections
    client_names[output[0]] = 0;
  }

  // Client 2
  ret = read( client_socket[1], &output[0], BUFFER_SIZE-1 );
  if ( ret > 0 ) {
    cout << "Server received from 2:  " << output << endl;
    client_names[output[0]] = 1;
  }
}

// Implmentation of sending data over cpp socket.
// Input; Data; the data to be sent. Socket; the socket we will be sending over.
void
SyncSocket::sendData( std::string data, char socket ) {

  json packet;
  int i;
  int n = ( data.size()/700 ) + 1;
  std::string chunks[n];
  if (data  == "null")
	  chunks[0] = "{}";
  else{
     // Find number of chunks.
     for ( i = 0; i<n; i++ ) {
       chunks[i] = data.substr( i*700, 700 );
     }
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
    //std::cout << " ========PJ: " << pj << std::endl;

    i += 1;
    ret = write( client_socket[client_names[socket]],&pj[0],pj.size() );

    if ( ret < 0 ) {
      cout << "Write failed" << endl;
    }

  }
}


// Implmentation for receiving data packet.
// Input: Socket; the reciving socket ( for OpenDSS or ReDis-PV ).
std::string
SyncSocket::receiveData( char socket ) {

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
    ret = read( client_socket[client_names[socket]], &buff[0], BUFFER_SIZE-1 );

    if ( ret > 0 && ( ret != 1 || buff[0] != ' ' ) ) {
      //cout << "Confirmation code  " << ret << endl;
      //cout << "Server received:  " << output << endl;

      while ( ret < 1023 ) {
        int pret = read( client_socket[client_names[socket]], &buff[ret], BUFFER_SIZE-( ret +1 ) );
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

}
}
