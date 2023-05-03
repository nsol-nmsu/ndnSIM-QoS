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

#include <unordered_map>

namespace ns3 {
namespace ndn {

class Synchronizer{
public:
  /** \brief Constructor 
    */
  Synchronizer();

  
  /** \brief Pause te simulation and perform synching operations
   */
  virtual void
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

  void
  beginSync(double seconds);  

  std::vector<std::string>
  SplitString(std::string strLine, int limit);

  virtual void
  sendSync()=0;

  virtual void
  receiveSync()=0;


private:  

  void
  printListAndTime();

protected:

  //std::unordered_map<int,Ptr<ConsumerQos>> senders;

  std::vector<std::string> arrivedPackets;

  std::vector<std::string> packetNames;

  double timestep;

};

}
}
#endif

