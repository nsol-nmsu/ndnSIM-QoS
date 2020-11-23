#ifndef PARSER_REDISPV_H
#define PARSER_REDISPV_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "nlohmann/json.hpp"

#define BUFFER_SIZE 1024

namespace ns3 {
namespace ndn {
using json = nlohmann::json;

class ParserReDisPv {

public:

  ParserReDisPv(  );

  void
  processJson( json njf, std::vector<std::string>* packetNames, int* leads );

  void 
  setRefs( std::unordered_map<std::string,int> *nMap, std::unordered_map<std::string,std::string> *DERmap ) {
    nameMap = nMap;
    mapDER = DERmap;
  }

  void setPVNode( int node ) {
    PVNode = node;
  };

private:

  std::unordered_map<std::string,int> *nameMap;
  std::unordered_map<std::string,std::string> *mapDER;
  int PVNode;

};

}
}
#endif

