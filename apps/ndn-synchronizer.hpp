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

#ifndef NDN_SYNCHRONIZER_H
#define NDN_SYNCHRONIZER_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"

#include "ndn-QoS-consumer.hpp"
#include <unordered_map>

namespace ns3 {
namespace ndn {

class Synchronizer{
public:
  /** \brief Constructor 
    */
  Synchronizer();

  /** \brief Add  a reference to the consumer application of a sending node
   *  \param node the node to which the appliaction belongs to
   *      \param sender reference to the appliaction instance
   */
  void
  addSender(int node, Ptr<ConsumerQos> sender);
  
  /** \brief Pause te simulation and perform synching operations
   */
  void
  syncEvent();

  /** \brief Save information on recieved packet
   *  \param string string with information on the arriving interest.
   */
  virtual void 
  addArrivedPackets(std::string);

  /** \brief Set how often we pause the simulation to perform sync operations
   *  \param step seconds between sync operations
   */
  void
  setTimeStep(double step);

  /** \brief Begin synching operations
  */ 

  void
  beginSync();

  /** \brief inject interests as descibed by th packetNames vector
   *  \param agg should interests at the same node be aggregated
   */

   void
  injectInterests(bool agg, bool set);

  void
  DDoSMode(bool set){
     m_dos = set;
     if(!m_dos)
    	 for ( auto it = attackers.begin(); it != attackers.end(); ++it ){
    	    	   it->second->setTarget("/");
    	    	   it->second->setAttackRate(0);
    	    }
  };

  void
  StartAttack(double sec) {
     Simulator::Schedule( Seconds( sec), &Synchronizer::DDoSMode, this, true );
  };

  void
  EndAttack(double sec) {
     Simulator::Schedule( Seconds( sec), &Synchronizer::DDoSMode, this, false );
  };

  std::vector<std::string>
  SplitString(std::string strLine, int limit);


  void
  addAttackers(int node, Ptr<ConsumerQos> attacker);

  virtual void
  sendSync()=0;

  virtual void
  receiveSync()=0;

  virtual void
  injectAttack()=0;

private:  

  void
  printListAndTime();

protected:

  std::unordered_map<int,Ptr<ConsumerQos>> senders;

  std::unordered_map<int,Ptr<ConsumerQos>> attackers;

  std::vector<std::string> arrivedPackets;

  std::vector<std::string> packetNames;

  double timestep;

  bool m_dos = false;
  int m_dosRate = 1000; //Packets per seconds
  double m_leadRate = 0.5; //Percentage of leads targeted
};

}
}
#endif

