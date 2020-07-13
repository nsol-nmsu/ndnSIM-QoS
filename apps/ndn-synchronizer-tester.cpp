#include "ndn-synchronizer-tester.hpp"
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
#include <iostream>
#include <iomanip>
#include <fstream>
#include "nlohmann/json.hpp"



namespace ns3 {
namespace ndn {

using json = nlohmann::json;
  
SyncTester::SyncTester(){

	//     timestep = 0;
	//     Simulator::Schedule( Seconds( 0.0 ), &Synchronizer::syncEvent, this );
	packets = "1";
}	  


void
SyncTester::sendSync(){

	std::cout<<"From tester "<<std::endl;

	while ( !arrivedPackets.empty() ) {
		std::cout<<arrivedPackets.back()<<std::endl;
		arrivedPackets.pop_back();
	}

	//packetNames.push_back( packets );
}


void   
SyncTester::receiveSync(){

	std::ifstream ifs( "/home/george/NDN_QoS/ns-3/test.json" );
	json jf = json::parse( ifs );

	std::cout << jf << '\n';
	std::cout << jf.dump( 4 ) << '\n';

	json::iterator it = jf.begin();
	std::cout << it.key() << " : " << it.value() << "\n";

	if ( jf.contains( "len" ) ) {
		std::cout << jf.contains( "len" ) << "\n";
	}

	packetNames.push_back( packets );
}


}
}
