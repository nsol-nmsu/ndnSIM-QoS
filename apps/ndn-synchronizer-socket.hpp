#ifndef NDN_SYNCHRONIZER_SOCKET_H
#define NDN_SYNCHRONIZER_SOCKET_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "nlohmann/json.hpp"

#include "ndn-synchronizer.hpp"
#define BUFFER_SIZE 1024

namespace ns3 {
namespace ndn {

class SyncSocket :  public Synchronizer {

public:

  SyncSocket();

  void
  addArrivedPackets( std::string );

  virtual void
  sendSync();

  virtual void 
  receiveSync();

  void
  processJson();	  

  void
  fillNameMap( nlohmann::json jf );

  void
  initializeJson( nlohmann::json jf );

private:

  int ret;
  int server_socket;
  int client_socket;
  char buffer[ BUFFER_SIZE ];
  std::unordered_map<std::string,int> nameMap;
  nlohmann::json rjf;
  nlohmann::json njf;

};

}
}
#endif

