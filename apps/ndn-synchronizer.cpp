#include "ndn-synchronizer.hpp"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/integer.h"
#include "ns3/double.h"

#include "apps/ndn-app.hpp" // This header is required for Trace Sink
namespace ns3 {
namespace ndn {

  
Synchronizer::Synchronizer(){
	timestep = 0;
}	  


void
Synchronizer::addSender(int node, Ptr<ConsumerQos> sender){
	senders[node] = sender;
}

void
Synchronizer::printListAndTime(){
	std::cout<<"Current Time is: "<<Simulator::Now()<<std::endl;
	Simulator::Schedule(Seconds(1.0), &Synchronizer::printListAndTime, this);
}

void
Synchronizer::setTimeStep(double step){
	timestep = step;
}

void
Synchronizer::addArrivedPackets(std::string str){
	arrivedPackets.push_back(str);
}

void
Synchronizer::beginSync(){
	Simulator::Schedule(Seconds(0.0), &Synchronizer::syncEvent, this);
}

void
Synchronizer::syncEvent(){

	std::cout<<"Syncing at time "<<Simulator::Now().GetSeconds()<<std::endl;
	//std::unordered_map<std::string,int>::iterator nm = nameMap.begin();
	//while (nm != nameMap.end()) {
	//	std::cout<<nm->first<<": "<<nm->second<<"  ";
	//	nm++;
	//}
	//std::cout<<std::endl;
	//std::unordered_map<int,Ptr<ns3::ndn::ConsumerQos>>::iterator itt = senders.begin();
	sendSync();
	receiveSync();

	//int count = std::stoi(packetNames.back());
	//packetNames.pop_back();

	//while (itt != senders.end()) {
	//	int i = 0;
	//  while (i<count) {
	//  	Simulator::ScheduleWithContext(itt->first,Seconds(i*(timestep/count)), &ConsumerQos::SendPacket, senders[itt->first]/*, packetNames.back()*/);
	//	   	i++;
	//	}
	//	itt++;
	//}
	//std::cout<<packetNames.size()<<std::endl;

	while (!packetNames.empty()) { 
		std::vector<std::string> packetInfo = SplitString( packetNames.back() );
		//std::cout<<packetInfo[1]<<" "<<packetInfo[0]<<std::endl;
		Simulator::ScheduleWithContext(std::stoi(packetInfo[1]),Seconds(0.0), &ConsumerQos::SendPacket, senders[std::stoi(packetInfo[1])], packetInfo[0], packetInfo[2] );
		packetNames.pop_back();
	}	

	Simulator::Schedule(Seconds(timestep), &Synchronizer::syncEvent, this);
}


std::vector<std::string> 
Synchronizer::SplitString( std::string strLine ) {

	std::string str = strLine;
	std::vector<std::string> result;
	std::istringstream isstr( str );
	int i =0;
	std::string finalStr = "";

	for ( std::string str; isstr >> str;  ) {

		if ( i<2 ) {     
			result.push_back( str );
		} else { 
			finalStr += str;
		}

		i++;
	}

	result.push_back( finalStr );

	return result;
}

}	// end of namespace ndn
}	// end of namespace ns3

