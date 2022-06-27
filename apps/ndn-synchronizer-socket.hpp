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

#ifndef NDN_SYNCHRONIZER_SOCKET_H
#define NDN_SYNCHRONIZER_SOCKET_H

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "nlohmann/json.hpp"

#define BUFFER_SIZE 1024

namespace ns3 {
namespace ndn {

class SyncSocket {

public:

  /** \brief  On object construction create sockets and set references.
   */
  SyncSocket();


  void
  start();
  /** \brief Implmentation of sending data over cpp socket.
   *  \param Data the data to be sent
   *  \param Socket the socket we will be sending on.
   */
  void
  sendData( std::string data, char socket );


  /** \brief Implmentation for receiving data packet.
   *  \param Socket the reciving socket ( for OpenDSS or ReDis-PV ).
   */
  std::string
  receiveData( char socket );


private:

  int ret;
  int server_socket;
  vector<int> client_socket;
  unordered_map<char, int> client_names;
  char buffer[ BUFFER_SIZE ];


};

}
}
#endif

