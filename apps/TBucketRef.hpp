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

#ifndef NDN_TOKENBUCKETS_H
#define NDN_TOKENBUCKETS_H

#include "NFD/daemon/fw/ndn-token-bucket.hpp"
#include <unordered_map>

using namespace std;

namespace nfd {
namespace fw {

/**
 * @ingroup ndnQoS
 * @brief Links application layer to strategy layer for the token bucket.
 *
 * Provides tokenBucketDriver class references to the tokenBucket objects as defined by qos-strategy instances.
 */
struct  TBRef {
  double m_tokens;
  double m_need;
  unordered_map<int,bool> hasSender;

  unordered_map<int,TokenBucket*> sender1;  
  unordered_map<int,TokenBucket*> sender2;
  unordered_map<int,TokenBucket*> sender3;
};

extern TBRef CT;

}
}
#endif
