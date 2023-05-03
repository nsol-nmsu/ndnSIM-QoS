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
#include "ndn-synchronizer-DOE.hpp"

using namespace std;

namespace ns3 {
namespace ndn {

using json = nlohmann::json;

double
ParserOpenDSS::processJson( json njf, std::vector<std::string>* packetNames ) {

  json::iterator it = njf.begin();
  double time = 0.0;
  int count = 0;
  // Loop through received Json file and create packets for sending over ndnSIM.
  while ( it !=  njf.end() ) {

    int consumer;
    std::string device;
    std::string packet_insert;
    int payloadSize = 150;
    //std::cout<<it.key()<<std::endl;
    if ( it.key() == "Time"){
       time = double(it.value());
    }

    else if ( it.key() == "Switch" || it.key() == "Transformer" || it.key() == "Capacitor" || it.key() == "PVSystem" 
		    || it.key() == "Storage" || it.key() == "Load"|| it.key() == "Line" ) {
      json::iterator it1 = it.value().begin();

      while ( it1 != it.value().end() ) {
        //std::cout<<"Obtaining Device Name\n";
	if ( it.key() == "Switch" || it.key() == "Line"){
		consumer = ( *nameMap ).begin()->second;
	}
	else 
	consumer = ( *nameMap )[it1.key()];
        //std::cout<<"Name: "<<it1.key()<<" Node: "<<consumer<<" " << it.key() << std::endl;
        device = it1.key();

        /*if(it1.value().begin() == it1.value().end()){
           it1++;
           continue;
        }*/
        json::iterator it2 = it1.value().begin();
        std::string payload = it1.value().dump();
        payloadSize = payload.size();

        while ( it2 != it1.value().end() ) {

          if ( it2.key() == "busdata" ) {
          }

          it2++;
        }

        if ( mapDER->find( it1.key() ) != mapDER->end() ) {
          std::string nodeNumber = std::to_string( ( *nameMap )[( *mapDER )[it1.key()]] );
          std::string leadDER = ( *mapDER )[it1.key()];
          packet_insert = "phy" + nodeNumber + "/" + leadDER + "/data/" + device + " " + std::to_string( consumer ) +  " " + payload;
        } else {
          packet_insert = "data/" + device + " " + std::to_string( consumer ) +  " " + payload;
        }

        packetNames->push_back( packet_insert );
        it1++;
      }
    }

    it++;
  }
  return time;
}

}
}

