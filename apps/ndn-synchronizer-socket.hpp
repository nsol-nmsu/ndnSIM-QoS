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

  bool
  sendDirect( std::string send_json, int src );

  virtual void
  sendSync();

  void
  sendData( std::string data, int socket );

  virtual void 
  receiveSync();

  std::string
  receiveData( int socket );

  void
  processJson();	  

  void
  fillNameMap( nlohmann::json jf );

  void
  initializeJson( nlohmann::json jf );

  void
  processRPVJson( nlohmann::json jf );

  void
  processLeadJson( nlohmann::json jf, int src );

  void setPVNode( int node ) {
     PVNode = node;
  };

private:

  int ret;
  int server_socket;
  int client_socket[ 2 ];
  int OpenDSS;
  int RedisPv;
  int PVNode;
  int leads=0;
  char buffer[ BUFFER_SIZE ];
  std::unordered_map<std::string,int> nameMap;
  nlohmann::json rjf;
  nlohmann::json njf;

};

}
}
#endif

