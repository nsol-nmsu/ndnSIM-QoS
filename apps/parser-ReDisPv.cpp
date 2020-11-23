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
