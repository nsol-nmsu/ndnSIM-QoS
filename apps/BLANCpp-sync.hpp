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

#ifndef BLANCPP_SYNCH_DOE_H
#define BLANCPP_SYNCH_DOE_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "ndn-synchronizer.hpp"

#include "ns3/BLANC++.hpp"

namespace ns3 {
namespace ndn {

class BLANCPPSync :  public Synchronizer {

public:

  /** \brief  On object construction create sockets and set references.
   */
  BLANCPPSync();


  virtual void
  syncEvent() override;


  /** \brief Add a string to list of packets that arrived at their distination.
   *  \param Str string with information on the arriving packet.
   */
  void
  onFindReplyPacket( uint32_t node, uint32_t txID, double amount);

  void
  onHoldPacket( uint32_t node, uint32_t txID, bool received);

  void
  onPayPacket( uint32_t node, uint32_t txID);

  void
  onTxFail(uint32_t txID);

  void
  injectPackets( ) ;

  void
  createTables( ) ;

  /** \brief Add  a reference to the consumer application of a sending node
   *  \param node the node to which the appliaction belongs to
   *      \param sender reference to the appliaction instance
   */
  void
  addNode( int node, Ptr<BLANCpp> app );

  void
  addSender( int node);  

  void
  addRH( int node, Ptr<BLANCpp> routingHelper );

  virtual void
  sendSync(){};

  /** \brief Receive data from co-simulators.
   */
  virtual void
  receiveSync(){};

  void setFindTable(uint32_t node, std::string RH, std::string nextHop){
     nodes[node]->setFindTable(RH, nextHop);
  };

  void setNeighborCredit(uint32_t node, std::string name, double amount){
     nodes[node]->setNeighborCredit(name, amount);
  };

  void setAddressTable(int node, std::string name, Ipv4Address address){
      nodes[node]->setNeighbor(name, address);
  };

private:


  std::unordered_map<int,Ptr<BLANCpp>> nodes;
  std::unordered_map<int,Ptr<BLANCpp>> senders;
  std::unordered_map<int,Ptr<BLANCpp>> routingHelpers;

  std::unordered_map<uint32_t, std::vector <uint32_t>> txIDmap;
  std::unordered_map<uint32_t, uint32_t> T_SMap;
  std::unordered_map<uint32_t, std::vector <bool>> FRMap;
  std::unordered_map<uint32_t, bool> HRMap;
  std::unordered_map<uint32_t, bool> HoldMap;

  bool m_tablesMade = false;

};

}
}
#endif

