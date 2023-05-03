/*
 * Copyright ( C ) 2020 New Mexico State University
 *
 * George Torres, Anju Kunnumpurathu James
 * See AUTHORS.md for complete list of authors and contributors.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * ( at your option ) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef NDN_SYNCHRONIZER_LOSS_H
#define NDN_SYNCHRONIZER_LOSS_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "nlohmann/json.hpp"
#include "ndn-synchronizer.hpp"
#include "ndn-synchronizer-socket.hpp"
#include "parser-OpenDSS.hpp"
#include "parser-ReDisPv.hpp"

#include "ndn-QoS-consumer.hpp"


#define BUFFER_SIZE 1024

namespace ns3 {
namespace ndn {

class SyncLoss :  public Synchronizer {

public:

  /** \brief  On object construction create sockets and set references.
   */
  SyncLoss();

  virtual void
  syncEvent() override;


  /** \brief Add a string to list of packets that arrived at ReDis-PV and follower DERs.
   *  \param Str string with information on the arriving interest.
   */
  void
  addArrivedPackets( std::string );

  /** \brief Process packets with set point data, that arrive at a follwer DER or a lead DER and injectnew interest as needed.
   *  \param Send_json the data we will be sending. 
   *  \param Src the source node from which interests will be sent.
   *  \param Device Name the name of the reciving interest.
   */
  bool
  sendDirect( std::string send_json, int src, std::string deviceName );

  /** \brief When packet arrives at Lead DER from follower DER, update Lead payload and send new packet to ReDis-PV.
   *  \param payload the payload of the arriving interest. 
   *  \param follower the source node for interest 
   *  \param lead the node the recived the interest
   */
  void
  aggDER( std::string payload, int src, std::string follower, std::string lead );

  /** \brief Send infromation gathered over span of the timestep to the co-simulators.
   */
  virtual void
  sendSync();

  /** \brief Receive data from co-simulators.
   */
  virtual void 
  receiveSync();


  /** \brief inject interests as descibed by th packetNames vector
   *  \param agg should interests at the same node be aggregated
   */
  void
  injectInterests( bool agg, bool set ) {
     double seconds = 0.0;
     double step = 0.0006;
     while ( !packetNames.empty() ) {
        std::vector<std::string> packetInfo = SplitString( packetNames.back(), 2 );
        Simulator::ScheduleWithContext( std::stoi( packetInfo[1] ), Seconds( seconds ), &ConsumerQos::SendPacket, 
			senders[std::stoi( packetInfo[1] )], packetInfo[0], packetInfo[2], agg, set );
	seconds += step;
        packetNames.pop_back();
     }
  };


  /** \brief Add  a reference to the consumer application of a sending node
   *  \param node the node to which the appliaction belongs to
   *      \param sender reference to the appliaction instance
   */
  virtual void
  addSender( int node, Ptr<ConsumerQos> sender ) {
     senders[node] = sender;
  };


  /** \brief Fill out hash map where we link deivce names to ns-3 nodes.
   *  \param  Json has information on what names go with what device.
   */
  void
  fillNameMap( nlohmann::json jf );

  /** \brief Create the first instance of the running measumernt Json.
   *  \param Json An instance of th measurment data..
   */
  void
  initializeJson( nlohmann::json jf );

  /** \brief Process Json file from Lead DER ( openDSS ) and create appropriate packets.
   */
  void
  processLeadJson( nlohmann::json jf, int src, std::string srcDevice );

  /** \brief Set postion of the Controller node
   *  \param node the node which is designated as the lead  controller.
   */
  void setPVNode( int node ){
     PVNode = node;
     parRedis.setPVNode( node );
  };

  int
  getNodeFromName(std::string name){
     return nameMap[name];	  
  };

private:

  int PVNode;
  int leads=0;
  std::unordered_map<std::string,int> nameMap;
  std::unordered_map<std::string,std::string> mapDER;
  std::unordered_map<std::string,std::string> leadToFolDER;
    std::unordered_map<std::string,std::vector<std::string>> leadMap;
  std::unordered_map<std::string,bool> LeadDERs;
  std::unordered_map<int,Ptr<ConsumerQos>> senders;
  std::unordered_map<int,bool> dropMap;
  double endDrop = 0.0;


  ParserOpenDSS parOpen;
  ParserReDisPv parRedis;
  SyncSocket socket;
  nlohmann::json rjf;
  nlohmann::json rSend;
  nlohmann::json OSendTemp;
  nlohmann::json njf;

};

}
}
#endif

