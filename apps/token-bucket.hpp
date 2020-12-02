/*
 * Copyright ( C ) 2020 New Mexico State University
 *
 * George Torres, Anju Kunnumpurathu James
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

#include "ns3/nstime.h"
#include "ns3/ptr.h"

namespace ns3 {
namespace ndn {

/**
 * @ingroup ndn-apps
 * @brief A simple app meant to produce tokens
 * Generates tokesn at a given frequency and updates the 
 * appropriate token buckets
 */
class TBucket : public App {

public:

  static TypeId
  GetTypeId(void);

  TBucket();

  /** \brief Schedule the next instance of token generation for the indicated bucket.
   *  \param bucket the bucket we are scheduling token generation for.
   */
  void
  ScheduleNextToken( int bucket );

  /** \brief Generate a new token for the indicated bucket.
   *  \param bucket the bucket we are generating the token for.
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
  double m_capacity1;
  double m_capacity2;
  double m_capacity3;
  double m_tokens;
  double m_fillRate1;
  double m_fillRate2;
  double m_fillRate3;
  bool m_first1;
  bool m_first2;
  bool m_first3;
  bool m_connected;
};

} // namespace ndn
} // namespace ns3

#endif // NDN_TOKEN_H
