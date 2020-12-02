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

#ifndef PARSER_OPENDSS_H
#define PARSER_OPENDSS_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "nlohmann/json.hpp"

#define BUFFER_SIZE 1024

namespace ns3 {
namespace ndn {
using json = nlohmann::json;

class ParserOpenDSS {

public:

  /** \brief Constuctor
   */
  ParserOpenDSS() {};

  /** \brief Process measurment Json from OpenDSS 
   *  \param njf json file recieved from OpenDSS
   *  \param packetNames reference to the vector to which we will save generated packets to
   */
  void
  processJson( json njf, std::vector<std::string>* packetNames );

  /** \brief Set the references need to operate parsing succesfully
   *  \param nMap mapp containing the names of all devices and thier locations
   *  \param DERmap references to map that keeps track of lead DERs and thier followers
   */
  void 
  setRefs( std::unordered_map<std::string,int> *nMap, std::unordered_map<std::string,std::string> *DERmap ) {
    nameMap = nMap;
    mapDER = DERmap;
  }

  std::unordered_map<std::string,int> *nameMap;
  std::unordered_map<std::string,std::string> *mapDER;

};
}
}
#endif

