#ifndef NDN_SYNCHRONIZER_H
#define NDN_SYNCHRONIZER_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"

#include "ndn-subscriber-sync.hpp"
#include <unordered_map>

namespace ns3 {
namespace ndn {

class Synchronizer{
public:
  Synchronizer();

  void
  addSender(int node, Ptr<ConsumerQos> sender);
  
  void
  syncEvent();

  virtual void 
  addArrivedPackets(std::string);

  void
  setTimeStep(double step);

  void
  beginSync();

  std::vector<std::string>
  SplitString( std::string strLine );

  virtual void
  sendSync()=0;

  virtual void 
  receiveSync()=0;

private:  

  void
  printListAndTime();

  std::unordered_map<int,Ptr<ConsumerQos>> senders;

protected:

  std::vector<std::string> arrivedPackets;

  std::vector<std::string> packetNames;

  double timestep;
};

}
}
#endif

