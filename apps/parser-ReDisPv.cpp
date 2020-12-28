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
#include "ndn-synchronizer-socket.hpp"

#define DEBUG  0

using namespace std;

namespace ns3 {
namespace ndn {

using json = nlohmann::json;


ParserReDisPv::ParserReDisPv(){
}


void
ParserReDisPv::processJson( json njf, std::vector<std::string>* packetNames, int* leads ) {

  json::iterator it = njf.begin();

  while ( it !=  njf.end() ) {
    ( *leads )++;
    std::string device;
    std::string payload;
    int payloadSize;
    std::string packet_insert;
    json::iterator it1 = it.value().begin();

    while ( it1 !=  it.value().end() ) {

      if ( it1.key() == "Lead_DER" ) {
        device = "phy" + std::to_string( ( *nameMap )[it1.value()] ) + "/";
        device += it1.value();
      } else {
        payload = it1.value().dump();
        payloadSize = payload.size();
      }

      it1++;
    }

    packet_insert = device + " " + std::to_string( PVNode ) +  " " + payload;

    if ( DEBUG )   std::cout << packet_insert << std::endl;

    packetNames->push_back( packet_insert );
    it++;
  }
}

}
}
