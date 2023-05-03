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

#ifndef NDN_ATTACKER_REF_H
#define NDN_ATTACKER_REF_H

#include <unordered_map>
#include "core/common.hpp"

using namespace std;

namespace nfd {
namespace fw {

/**
 * @ingroup ndnQoS
 */
struct  AttackRef {
  std::unordered_map<int, bool> setMap;//node, set
  std::unordered_map<int, Name> targetMap;//node, target
  std::unordered_map<Name, int> popMap; //Name, finds
  int lastClear = 0;
};

extern AttackRef attacker_map;

}
}

#endif