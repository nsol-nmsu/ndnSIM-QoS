#ifndef NDN_SYNCHRONIZER_DDOS_H
#define NDN_SYNCHRONIZER_DDOS_H
#include "ndn-synchronizer-socket.hpp"
#include "ndn-synchronizer.hpp"
#include "nlohmann/json.hpp"
#include "../helper/ndn-region-helper.hpp"

namespace ns3 {
namespace ndn {
class SyncDDoS : public SyncSocket {

public:
        SyncDDoS(){};
        virtual void injectAttack() override;
	virtual void attackRedisPV();
};
}}
#endif
