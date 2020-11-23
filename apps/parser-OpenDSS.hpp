#ifndef PARSER_OPENDSS_H
#define PARSER_OPENDSS_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "nlohmann/json.hpp"

#define BUFFER_SIZE 1024

namespace ns3 {
namespace ndn {
using json = nlohmann::json;

class ParserOpenDSS {

public:

  ParserOpenDSS() {};

  void
  processJson(json njf, std::vector<std::string>* packetNames);

  void 
  setRefs(std::unordered_map<std::string,int> *nMap, std::unordered_map<std::string,std::string> *DERmap) {
    nameMap = nMap;
    mapDER = DERmap;
  }

  std::unordered_map<std::string,int> *nameMap;
  std::unordered_map<std::string,std::string> *mapDER;

};
}
}
#endif

