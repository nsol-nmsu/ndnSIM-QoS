/*Copyright (C) 2020 New Mexico State University
 *
 * George Torres, Anju Kunnumpurathu James
 * See AUTHORS.md for complete list of authors and contributors.
 *
 *This program is free software: you can redistribute it and/or modify
 *it under the terms of the GNU General Public License as published by
 *the Free Software Foundation, either version 3 of the License, or
 *(at your option) any later version.

 *This program is distributed in the hope that it will be useful,
 *but WITHOUT ANY WARRANTY; without even the implied warranty of
 *MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *GNU General Public License for more details.

 *You should have received a copy of the GNU General Public License
 *along with this program.  If not, see <http://www.gnu.org/licenses/>.
 **/

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

  
Synchronizer::Synchronizer() {
	timestep = 0;
}	  


void
Synchronizer::addSender( int node, Ptr<ConsumerQos> sender ) {

	senders[node] = sender;
}


void
Synchronizer::printListAndTime() {

	std::cout << "Current Time is: " << Simulator::Now() << std::endl;
	Simulator::Schedule( Seconds( 1.0 ), &Synchronizer::printListAndTime, this );
}


void
Synchronizer::setTimeStep( double step ) {
	timestep = step;
}


void
Synchronizer::addArrivedPackets( std::string str ) {
	arrivedPackets.push_back( str );
}


void
Synchronizer::beginSync() {
	Simulator::Schedule( Seconds(30.0 ), &Synchronizer::syncEvent, this );
}


void
Synchronizer::syncEvent() {

	std::cout << "\n\n\nSyncing at time " << Simulator::Now().GetSeconds() << std::endl;
	sendSync();
        //std::cout << "\n\n\nRecv " << Simulator::Now().GetSeconds() << std::endl;	
	receiveSync();

	injectInterests( false, false );
	Simulator::Schedule( Seconds( timestep ), &Synchronizer::syncEvent, this );

}


void
Synchronizer::injectInterests( bool agg, bool set ) {

	int i = 0;
	while ( !packetNames.empty() ) {

		std::vector<std::string> packetInfo = SplitString( packetNames.back(), 2 );
		//std::cout<<"PacketInfo " <<packetInfo[1]<<" "<<packetInfo[0]<<" "<<packetInfo[2]<<packetNames.size()<<std::endl;

		Simulator::ScheduleWithContext( std::stoi( packetInfo[1] ), Seconds( 0.0 ), &ConsumerQos::SendPacket, senders[std::stoi( packetInfo[1] )], packetInfo[0], packetInfo[2], agg, set );
		packetNames.pop_back();
		i++;
		if(i >= 100){
	   		Simulator::Schedule( Seconds( 0.0001 ), &Synchronizer::injectInterests, this, agg, set );
			return;
		}
	}
}


std::vector<std::string> 
Synchronizer::SplitString( std::string strLine, int limit ) {

	std::string str = strLine;
	std::vector<std::string> result;
	std::istringstream isstr( str );
	int i = 0;
	std::string finalStr = "";

	for ( std::string str; isstr >> str;  ) {

		if ( i < limit || limit == 0 ) {
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

