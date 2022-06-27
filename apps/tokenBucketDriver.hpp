/*
 * Copyright ( C ) 2020 New Mexico State University- Board of Regents
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

#ifndef NDN_TOKEN_H
#define NDN_TOKEN_H

#include "ns3/ndnSIM/model/ndn-common.hpp"

#include "ndn-app.hpp"
#include "ns3/ndnSIM/model/ndn-common.hpp"
#include "NFD/daemon/fw/ndn-token-bucket.hpp"

#include "ns3/nstime.h"
#include "ns3/ptr.h"

namespace ns3 {
namespace ndn {

/**
 * @ingroup ndnQoS
 * @brief App meant to define and drive token generation for traffic control.
 *
 * Defines token bucket capacity, and generation rate for strategy level token bucket objects;
 * one for each of the priority queues. Also sends token generation commands to token bucket 
 * at the given generation rate.
 *
 * Uses references provided by TBucketRef class in order to communicate with the 
 * token buckets that reside at the strategy level.
 */
class TBDriver : public App {

public:

  static TypeId
  GetTypeId(void);

  TBDriver();

  /** \brief Schedule the next instance of token generation for the indicated bucket.
   *  \param bucket The token bucket that we are scheduling for.
   */
  void
  ScheduleNextToken( int bucket );

  /** \brief Sends command to indicated token bucket to generate a new token.
   *  \param bucket The token bucket that we are sending the token generation command to.
   */
  void
  UpdateBucket( int bucket );

  void 
  addTokenBucket(nfd::fw::TokenBucket*);

  nfd::fw::TokenBucket*
  getBucket(int b){
	 return tbs[b];
  };


  bool 
  isSet(){
     return (m_bktsSet == m_bkts);	  
  };

  int
  getSize(){
     return m_bkts;
  };

  void
  setSize(int size){
     m_bkts = size;
  }

protected:

  EventId m_sendEvent; ///< @brief EventId of pending "send packet" event

  // Inherited from Application base class.
  virtual void
  StartApplication(); // Called at time specified by Start

  virtual void
  StopApplication(); // Called at time specified by Stop

private:

  std::string m_capacities; ///< @brief Defines token bucket capacities of priority levels, from highest to lowest.
  std::string m_fillRates; ///< @brief Defines token generation rate for priority levels, from highest to lowest.
  std::vector<bool> m_firsts; ///< @brief Boolean used to check if this is the first generated token for priority levels, from highest to lowest.
  std::vector<bool> m_tokenFilled;
  bool m_connected; ///< @brief Boolean used to check if all token bucket references are set.
  std::vector<nfd::fw::TokenBucket*> tbs;
  int m_bktsSet = 0;
  int m_bkts = 3;
  int m_MaxBkts = 4;



  std::vector<std::string>
  SplitString( std::string strLine, char delimiter ) {
     std::string str = strLine;
     std::vector<std::string> result;
     uint32_t i =0;
     std::string buildStr = "";

     for ( i = 0; i<str.size(); i++) {
        if ( str[i]== delimiter ) {
           result.push_back( buildStr );
           buildStr = "";
        }
        else {
           buildStr += str[i];
        }
     }

     if(buildStr!="")
        result.push_back( buildStr );

     return result;
  }
  
};

} // namespace ndn
} // namespace ns3

#endif // NDN_TOKEN_H
