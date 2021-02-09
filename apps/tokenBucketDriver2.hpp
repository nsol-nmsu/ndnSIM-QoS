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

#ifndef NDN_TOKEN2_H
#define NDN_TOKEN2_H

#include "ns3/ndnSIM/model/ndn-common.hpp"

#include "ndn-app.hpp"
#include "ns3/ndnSIM/model/ndn-common.hpp"

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
class TBDriver2 : public App {

public:

  static TypeId
  GetTypeId(void);

  TBDriver2();

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

protected:

  EventId m_sendEvent; ///< @brief EventId of pending "send packet" event

  // Inherited from Application base class.
  virtual void
  StartApplication(); // Called at time specified by Start

  virtual void
  StopApplication(); // Called at time specified by Stop

private:

  double m_capacity1; ///< @brief Defines token bucket capacity of priority level 1.
  double m_capacity2; ///< @brief Defines token bucket capacity of priority level 2.
  double m_capacity3; ///< @brief Defines token bucket capacity of priority level 3.
  double m_fillRate1; ///< @brief Defines token generation rate for priority level 1.
  double m_fillRate2; ///< @brief Defines token generation rate for priority level 2.
  double m_fillRate3; ///< @brief Defines token generation rate for priority level 3.
  bool m_first1; ///< @brief Boolean used to check if this is the first generated token for priority level 1. 
  bool m_first2; ///< @brief Boolean used to check if this is the first generated token for priority level 2.
  bool m_first3; ///< @brief Boolean used to check if this is the first generated token for priority level 3.
  bool m_connected; ///< @brief Boolean used to check if all token bucket references are set.
};

} // namespace ndn
} // namespace ns3

#endif // NDN_TOKEN_H
