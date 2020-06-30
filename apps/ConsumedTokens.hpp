#ifndef NDN_CONSUMEDTOKEN_H
#define NDN_CONSUMEDTOKEN_H
#include "NFD/daemon/fw/ndn-token-bucket.hpp"
#include <unordered_map>
using namespace std;
namespace nfd {
namespace fw {
struct  Consumed {
  double m_tokens;
  double m_need;
  unordered_map<int,bool> hasSender;

//  bool hasSender = false;
  unordered_map<int,TokenBucket*> sender1;  
  unordered_map<int,TokenBucket*> sender2;
  unordered_map<int,TokenBucket*> sender3;
  //TokenBucket *sender1;
  //TokenBucket *sender2;
  //TokenBucket *sender3;
};

extern Consumed CT;
}}
#endif
