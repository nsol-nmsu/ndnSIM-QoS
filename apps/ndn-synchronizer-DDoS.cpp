#include "ndn-synchronizer-DDoS.hpp"
#include "AttackerRef.hpp"
#include <algorithm>
#include <map>
#include <vector>
#include <set>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdlib.h>
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
#include "nlohmann/json.hpp"





using namespace std;

namespace ns3 {
namespace ndn {

void
SyncDDoS::syncEvent(){
   SyncDOE::syncEvent(); 
   if(m_dos){
      injectAttack();
   }
}

void
SyncDDoS::addAttackers( int node, Ptr<ConsumerQos> attacker ) {

        attackers[node] = attacker;
}

void 
SyncDDoS::injectAttack() {
	std::cout<<"$$ INJECT ATTACK"<<std::endl;
	//attack redispv
//	attackRedisPV();

	std::cout << "$$ Sync InjectAttack" << std::endl;
	std::vector<std::string> leads;      //Holds all currently known leads

//get top  interest
//
	std::map<ndn::Name, int> interestCount;

	for (auto it = nfd::fw::attacker_map.targetMap.begin();
	it != nfd::fw::attacker_map.targetMap.end(); it++) {
		//std::cout<<"Attacker_Map "<<it->second<<std::endl;
		if (interestCount.find(it->second) != interestCount.end()) {
			interestCount.find(it->second)->second++;
		} else {
			interestCount.insert(std::make_pair(it->second, 1));
	}

	}

	std::vector<std::pair<Name, int>> topFiveDevices(5);
	/*std::partial_sort_copy(interestCount.begin(), interestCount.end(),
			topFiveDevices.begin(), topFiveDevices.end(),
			[](std::pair<const Name, int> const &l,
					std::pair<const Name, int> const &r) {
				return l.second > r.second;
			});*/

	//testing the contents of popMap
	//for(auto pm =nfd::fw::attacker_map.popMap.begin(); pm!=nfd::fw::attacker_map.popMap.end();pm++){
//		std::cout<<"POPMAP "<<pm->first<<" count "<<pm->second<<std::endl;
	//}

	std::partial_sort_copy(nfd::fw::attacker_map.popMap.begin(), nfd::fw::attacker_map.popMap.end(),
                        topFiveDevices.begin(), topFiveDevices.end(),
                        [](std::pair<const Name, int> const &l,
                                        std::pair<const Name, int> const &r) {
                                return l.second > r.second;
                        });

	std::set<Name> nameList;
	for (auto const &pair : topFiveDevices) {
		std::cout << "{Top 5 Devices" << pair.first << ": " << pair.second << "}\n";
		nameList.insert(pair.first);
	}
	std::cout << "{Top 5 Devices}\n";
	//Fill in leads vector
	for (auto it = nfd::fw::attacker_map.popMap.begin();
			it != nfd::fw::attacker_map.popMap.end(); it++) {
				if (nameList.find(it->first) != nameList.end()) {
					std::cout << "{$ attack for " << it->first.toUri() << std::endl;
					leads.push_back(it->first.toUri().substr(1));
				}
	}

	int size = leads.size();



	//std::cout << "$$ attacker running. Leads size = "<<size << std::endl;
        int count = 0;
	std::string prefix = "/power/typeII/phy";
	//Randomly assign lead and rate to each attacker
	for (auto it = attackers.begin(); it != attackers.end(); it++) {
		if (size == 0)
			return;
		int t = rand() % size;
		std::cout << it->first << std::endl;
		std::string nodeNumber = std::to_string(getNodeFromName(leads.at(t)));

		std::string target = prefix + nodeNumber + "/" + leads.at(t)
				+ "/" + std::to_string(count++) + "/attack";
		it->second->setTarget(target);

		double rate = 800.0 + double(rand() % 100);
		rate = 1.0 / rate;
		it->second->setAttackRate(rate);
		std::cout<<"Top Attacking Lead at "<<leads.at(t)<<" Target is "<<target<<std::endl;



	}

}

void SyncDDoS::attackRedisPV() {
	std::cout<<"Attack to RedisPV initiated"<<std::endl;
	std::vector<std::string> redisPVAttackers;
	//123
	/*redisPVAttackers.push_back("17");
	redisPVAttackers.push_back("20");
	redisPVAttackers.push_back("27");
	redisPVAttackers.push_back("30");
	redisPVAttackers.push_back("31");
*/
	//650
/*
	redisPVAttackers.push_back("10");
	redisPVAttackers.push_back("11");
	redisPVAttackers.push_back("12");
	redisPVAttackers.push_back("13");
	redisPVAttackers.push_back("27");
	redisPVAttackers.push_back("28");
	redisPVAttackers.push_back("29");
	redisPVAttackers.push_back("30");
	redisPVAttackers.push_back("31");
	redisPVAttackers.push_back("33");
	redisPVAttackers.push_back("34");
	redisPVAttackers.push_back("35");
	redisPVAttackers.push_back("37");
	redisPVAttackers.push_back("39");
	redisPVAttackers.push_back("40");
	redisPVAttackers.push_back("42");
	redisPVAttackers.push_back("53");
	redisPVAttackers.push_back("55");
	redisPVAttackers.push_back("59");
	redisPVAttackers.push_back("60");
	redisPVAttackers.push_back("66");
	redisPVAttackers.push_back("89");
	redisPVAttackers.push_back("90");
	redisPVAttackers.push_back("95");
	redisPVAttackers.push_back("97");
	redisPVAttackers.push_back("134");
	redisPVAttackers.push_back("137");
	redisPVAttackers.push_back("138");
	redisPVAttackers.push_back("643");
	redisPVAttackers.push_back("144");
	redisPVAttackers.push_back("146");
	redisPVAttackers.push_back("149");
	redisPVAttackers.push_back("185");
	redisPVAttackers.push_back("189");
	redisPVAttackers.push_back("190");
	redisPVAttackers.push_back("191");
	redisPVAttackers.push_back("192");
	redisPVAttackers.push_back("193");
	redisPVAttackers.push_back("195");
	redisPVAttackers.push_back("196");
	redisPVAttackers.push_back("197");
	redisPVAttackers.push_back("212");
	redisPVAttackers.push_back("215");
	redisPVAttackers.push_back("218");
	redisPVAttackers.push_back("222");
	redisPVAttackers.push_back("227");
	redisPVAttackers.push_back("232");
	redisPVAttackers.push_back("233");
	redisPVAttackers.push_back("235");
	redisPVAttackers.push_back("574");
	redisPVAttackers.push_back("576");
	redisPVAttackers.push_back("581");
*/
	//2500
	redisPVAttackers.push_back("0");
	redisPVAttackers.push_back("1");
	redisPVAttackers.push_back("3");
	redisPVAttackers.push_back("4");
	redisPVAttackers.push_back("5");
	redisPVAttackers.push_back("6");
	redisPVAttackers.push_back("8");
	//redisPVAttackers.push_back("11");
	//redisPVAttackers.push_back("14");
	//redisPVAttackers.push_back("2465");
	//redisPVAttackers.push_back("15");
	redisPVAttackers.push_back("16");
	redisPVAttackers.push_back("23");
	//redisPVAttackers.push_back("19");
	//redisPVAttackers.push_back("20");
	//redisPVAttackers.push_back("24");
	//redisPVAttackers.push_back("25");
	redisPVAttackers.push_back("2463");
	redisPVAttackers.push_back("28");
	redisPVAttackers.push_back("29");
	//redisPVAttackers.push_back("30");
	//redisPVAttackers.push_back("31");
	//redisPVAttackers.push_back("2426");
	//redisPVAttackers.push_back("32");
	redisPVAttackers.push_back("33");
	redisPVAttackers.push_back("45");
	redisPVAttackers.push_back("34");
	redisPVAttackers.push_back("35");
	redisPVAttackers.push_back("36");
	redisPVAttackers.push_back("39");
	redisPVAttackers.push_back("41");
	redisPVAttackers.push_back("49");
	redisPVAttackers.push_back("2410");
	//redisPVAttackers.push_back("51");
	//redisPVAttackers.push_back("52");
	//redisPVAttackers.push_back("56");
	//redisPVAttackers.push_back("57");
	redisPVAttackers.push_back("58");
	redisPVAttackers.push_back("60");
	redisPVAttackers.push_back("61");
	redisPVAttackers.push_back("63");
	redisPVAttackers.push_back("2406");
	redisPVAttackers.push_back("66");
	redisPVAttackers.push_back("68");
	redisPVAttackers.push_back("757");
	redisPVAttackers.push_back("73");
	redisPVAttackers.push_back("74");
	redisPVAttackers.push_back("748");
	redisPVAttackers.push_back("78");
	/*redisPVAttackers.push_back("81");
	redisPVAttackers.push_back("83");
	redisPVAttackers.push_back("84");
	redisPVAttackers.push_back("91");
	redisPVAttackers.push_back("85");
	redisPVAttackers.push_back("86");
	redisPVAttackers.push_back("88");
	redisPVAttackers.push_back("92");
	redisPVAttackers.push_back("95");
	redisPVAttackers.push_back("101");
	redisPVAttackers.push_back("97");
	redisPVAttackers.push_back("102");
	redisPVAttackers.push_back("103");
	redisPVAttackers.push_back("106");
	redisPVAttackers.push_back("108");
	redisPVAttackers.push_back("109");
	redisPVAttackers.push_back("110");
	redisPVAttackers.push_back("111");
	redisPVAttackers.push_back("117");
	redisPVAttackers.push_back("120");
	redisPVAttackers.push_back("121");
	redisPVAttackers.push_back("126");
	redisPVAttackers.push_back("122");
	redisPVAttackers.push_back("124");
	redisPVAttackers.push_back("745");
	redisPVAttackers.push_back("729");
	redisPVAttackers.push_back("131");
	redisPVAttackers.push_back("132");
	redisPVAttackers.push_back("133");
	redisPVAttackers.push_back("723");
	redisPVAttackers.push_back("136");
	redisPVAttackers.push_back("233");
	redisPVAttackers.push_back("720");
	redisPVAttackers.push_back("137");
	redisPVAttackers.push_back("138");
	redisPVAttackers.push_back("139");
	redisPVAttackers.push_back("140");
	redisPVAttackers.push_back("141");
	redisPVAttackers.push_back("144");
	redisPVAttackers.push_back("145");
	redisPVAttackers.push_back("150");
	redisPVAttackers.push_back("148");
	redisPVAttackers.push_back("151");
	redisPVAttackers.push_back("153");
	redisPVAttackers.push_back("154");
	redisPVAttackers.push_back("156");
	redisPVAttackers.push_back("157");
	redisPVAttackers.push_back("158");
	redisPVAttackers.push_back("204");
	redisPVAttackers.push_back("159");
	redisPVAttackers.push_back("165");
	redisPVAttackers.push_back("161");
	redisPVAttackers.push_back("162");
	redisPVAttackers.push_back("168");
	redisPVAttackers.push_back("173");
	redisPVAttackers.push_back("175");
	redisPVAttackers.push_back("188");
	*/
	redisPVAttackers.push_back("178");
	redisPVAttackers.push_back("179");
	redisPVAttackers.push_back("183");
	redisPVAttackers.push_back("184");
	redisPVAttackers.push_back("189");
	redisPVAttackers.push_back("190");
	redisPVAttackers.push_back("191");
	redisPVAttackers.push_back("192");
	redisPVAttackers.push_back("193");
	redisPVAttackers.push_back("197");
	redisPVAttackers.push_back("199");
	redisPVAttackers.push_back("206");
	redisPVAttackers.push_back("214");
	redisPVAttackers.push_back("215");
	//redisPVAttackers.push_back("216");
	//redisPVAttackers.push_back("225");
	redisPVAttackers.push_back("218");
	//redisPVAttackers.push_back("219");
	//redisPVAttackers.push_back("220");
	redisPVAttackers.push_back("221");
	int size = redisPVAttackers.size();
	std::string prefix = "/power/typeII/data";
		//Randomly assign lead and rate to each attacker
		for (auto it = attackers.begin(); it != attackers.end(); it++) {
			if(std::find(redisPVAttackers.begin(),redisPVAttackers.end(),std::to_string(it->first))!=redisPVAttackers.end()){break;}
			if (size == 0)
				return;
			int t = rand() % size;
			//std::cout << it->first << std::endl;
			//std::string nodeNumber = std::to_string(nameMap[redisPVAttackers.at(t)]);
			std::cout<<"Attack to RedisPV Started"<<std::endl;
			std::cout << "Attacking Redispv from"<<it->first << std::endl;
			std::string target = prefix + "/" + redisPVAttackers.at(t)
					+ "/attack";
			it->second->setTarget(target);

			double rate = 800.0 + double(rand() % 200);
			rate = 1.0 / rate;
			it->second->setAttackRate(rate);

		}
}



  void
  SyncDDoS::DDoSMode(bool set){
     m_dos = set;
     if(!m_dos)
         for ( auto it = attackers.begin(); it != attackers.end(); ++it ){
                   it->second->setTarget("/");
                   it->second->setAttackRate(0);
            }
  }

  void
  SyncDDoS::StartAttack(double sec) {
     Simulator::Schedule( Seconds( sec), &SyncDDoS::DDoSMode, this, true );
  }

  void
  SyncDDoS::EndAttack(double sec) {
     Simulator::Schedule( Seconds( sec), &SyncDDoS::DDoSMode, this, false );
  }


}
}
