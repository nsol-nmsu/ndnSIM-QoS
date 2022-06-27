#ifndef NDN_SYNCHRONIZER_DDOS_H
#define NDN_SYNCHRONIZER_DDOS_H
#include "ndn-synchronizer-DOE.hpp"
#include "nlohmann/json.hpp"
#include "../helper/ndn-region-helper.hpp"

namespace ns3 {
namespace ndn {
class SyncDDoS : public SyncDOE {

public:

  SyncDDoS(){};
  
  virtual void
  syncEvent() override;

  void 
  injectAttack();
	
  void 
  attackRedisPV();

  void
  addAttackers( int node, Ptr<ConsumerQos> attacker );

  void
  DDoSMode(bool set);

  void
  StartAttack(double sec);

  void
  EndAttack(double sec);

protected:

  std::unordered_map<int,Ptr<ConsumerQos>> attackers;

  bool m_dos = false;
  int m_dosRate = 1000; //Packets per seconds
  double m_leadRate = 0.5; //Percentage of leads targeted
  

};
}}
#endif
