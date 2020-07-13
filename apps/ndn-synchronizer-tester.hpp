#ifndef NDN_SYNCHRONIZER_TESTER_H
#define NDN_SYNCHRONIZER_TESTER_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"

#include "ndn-synchronizer.hpp"

namespace ns3 {
namespace ndn {

class SyncTester :  public Synchronizer{

public:

  SyncTester();

  virtual void
  sendSync();

  virtual void 
  receiveSync();

  void
  numberOfPackets( int n ){
     packets=std::to_string( n );
  };

private:
  std::string packets;
};

}
}
#endif

