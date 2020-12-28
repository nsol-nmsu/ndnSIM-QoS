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

#ifndef PARSER_REDISPV_H
#define PARSER_REDISPV_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "nlohmann/json.hpp"

#define BUFFER_SIZE 1024

namespace ns3 {
namespace ndn {
using json = nlohmann::json;

class ParserReDisPv {

public:

  /** \brief Constuctor
   */
  ParserReDisPv();

  /** \brief Process Json file from ReDis-PV, creating needed packets as well.
   *  \param njf; the recived json file from ReDis-PV.
   */
  void
  processJson( json njf, std::vector<std::string>* packetNames, int* leads );


  /** \brief Set the references need to operate parsing succesfully.
   *  \param nMap mapp containing the names of all devices and thier locations.
   *  \param DERmap references to map that keeps track of lead DERs and thier followers.
   */
  void 
  setRefs( std::unordered_map<std::string,int> *nMap, std::unordered_map<std::string,std::string> *DERmap ) {
    nameMap = nMap;
    mapDER = DERmap;
  }


  /** \brief Set the ilocation of the lead controller.
   *  \param node noe on which the lead contoller is located.
   */
  void setPVNode( int node ) {
    PVNode = node;
  };

private:

  std::unordered_map<std::string,int> *nameMap;
  std::unordered_map<std::string,std::string> *mapDER;
  int PVNode;

};

}
}
#endif

