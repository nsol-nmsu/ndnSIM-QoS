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

#ifndef NDN_SYNCHRONIZER_SOCKET_H
#define NDN_SYNCHRONIZER_SOCKET_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "nlohmann/json.hpp"
#include "ndn-synchronizer.hpp"
#include "parser-OpenDSS.hpp"
#include "parser-ReDisPv.hpp"

#define BUFFER_SIZE 1024

namespace ns3 {
namespace ndn {

class SyncSocket :  public Synchronizer {

public:

  /** \brief  On object construction create sockets and set references.
   */
  SyncSocket();

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

  /** \brief Implmentation of sending data over cpp socket.
   *  \param Data the data to be sent
   *  \param Socket the socket we will be sending on.
   */
  void
  sendData( std::string data, int socket );

  /** \brief Receive data from co-simulators.
   */
  virtual void 
  receiveSync();

  /** \brief Implmentation for receiving data packet.
   *  \param Socket the reciving socket ( for OpenDSS or ReDis-PV ).
   */
  std::string
  receiveData( int socket );

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
  processLeadJson( nlohmann::json jf, int src );

  /** \brief Set postion of the Controller node
   *  \param node the node which is designated as the lead  controller.
   */
  void setPVNode( int node ){
     PVNode = node;
     parRedis.setPVNode( node );
  };

private:

  int ret;
  int server_socket;
  int client_socket[2];
  int OpenDSS;
  int RedisPv;
  int PVNode;
  int leads=0;
  char buffer[ BUFFER_SIZE ];
  std::unordered_map<std::string,int> nameMap;
  std::unordered_map<std::string,std::string> mapDER;
  std::unordered_map<std::string,std::string> leadToFolDER;
  std::unordered_map<std::string,bool> LeadDERs;
  ParserOpenDSS parOpen;
  ParserReDisPv parRedis;
  nlohmann::json rjf;
  nlohmann::json njf;

};

}
}
#endif

