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

#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include "BLANCpp-sync.hpp"
#include "ns3/ptr.h"
#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/string.h"
#include "ns3/boolean.h"
#include "ns3/uinteger.h"
#include "ns3/integer.h"
#include "ns3/double.h"

using namespace std;


namespace ns3 {
namespace ndn {

// On object construction create sockets and set references
BLANCPPSync::BLANCPPSync() {

}

void
BLANCPPSync::syncEvent(){
   Synchronizer::syncEvent();
   if(!m_tablesMade) createTables();
   injectPackets( );
}

bool DYNAMIC = true;
void
BLANCPPSync::injectPackets( ) {
//Go through list and find two apps not currently in a transaction. 
int sender, reciver;
std::string rh1, rh2;
if(DYNAMIC){

   int count = 0;
   vector<int> options;
   for(auto i = senders.begin(); i != senders.end(); i++){
      if (!nodes[i->first]->getTiP() && routingHelpers.find(i->first) == routingHelpers.end()){
         options.push_back(i->first);
      }
   }
   vector<int> pair;
   while (pair.size() < 2 && options.size() > 1){
      uint32_t choice = rand()%options.size();
        //std::cout<<"Router is "<<nodes[options[choice]]->getRH()<<std::endl;

      if ( count == 0 && nodes[ options[choice] ]->getRH() != "") {
         rh1 = nodes[ options[choice] ]->getRH();
         count++;
         pair.push_back(options[choice]);
      }
      else if (rh1 != nodes[ options[choice] ]->getRH() && 
            nodes[ options[choice] ]->getRH() != "") {
         //std::cout<<"RH is "<<nodes[ options[choice] ]->getRH()<<std::endl;
         rh2 = nodes[ options[choice] ]->getRH();
         count++;
         pair.push_back(options[choice]);
      }

      vector<int> copy = options;
      options.clear();
      for (int i = 0; i < copy.size(); i++){
         if(i != choice){
            options.push_back(copy[i]);
         }
      }
      
   }
   if (pair.size() != 2) {
      //std::cout<<"No pairs\n";
      return;
   }
   sender = pair[0];
   reciver = pair[1];
   //rh1 = "7";
   //rh2 = "1";
}  
else {
   if(nodes[0]->getTiP() == true){
      return;
   }
   sender = 0;
   reciver = 9;
   rh1 = "2";
   rh2 = "7";   
}
   uint32_t txID = rand()%10000000;
   uint32_t secret = rand()%10000000;
   ///std::cout<<"Starting tansaction "<<txID<<" "<<secret<<"\n\n"<<std::endl;
   txIDmap[txID].push_back(sender);
   txIDmap[txID].push_back(reciver);
   FRMap[txID].push_back(false);
   FRMap[txID].push_back(false);
   HRMap[txID] = false;
   HoldMap[txID] = false;
   std::vector<string> plist;
   plist.push_back(rh2);

   T_SMap[secret] = txID;

   nodes[reciver]->startTransaction(txID, secret, plist, false, 1);
   plist.clear();
   plist.push_back(rh1);
   plist.push_back(rh2);
   nodes[sender]->startTransaction(txID, secret, plist, true, 1);
}

void
BLANCPPSync::addNode( int node, Ptr<BLANCpp> app ) {
   nodes[node] = app;
}

void
BLANCPPSync::addSender( int node){
   senders[node] = nodes[node];
}

void
BLANCPPSync::addRH( int node, Ptr<BLANCpp> routingHelper ){
   routingHelpers[node] = routingHelper;
}

void
BLANCPPSync::createTables( ) {
   m_tablesMade = true;
   std::unordered_map<std::string, std::vector<std::string>> reachableMap;
   for(auto each = routingHelpers.begin(); each != routingHelpers.end(); each++){
      std::string node = std::to_string(each->first);
      reachableMap[node] = each->second->getReachableRHs();
   }
   std::unordered_map<std::string, std::vector<std::string>> reachableMap1;
   for(auto each = routingHelpers.begin(); each != routingHelpers.end(); each++){
      std::string node = std::to_string(each->first);
      reachableMap1[node] = each->second->matchUpNonces(reachableMap);
   }   
   for(auto each = routingHelpers.begin(); each != routingHelpers.end(); each++){
      each->second->setRHTable(reachableMap1);
   }
}

//TODO Change to Phase switching
void
BLANCPPSync::onFindReplyPacket( uint32_t node, uint32_t txID, double amount){
   txID = T_SMap[txID]; 

   //std::cout<<"Ummm "<<txID<<std::endl;

   if (txIDmap[txID][0] == node)
      FRMap[txID][0] = true;
   //else if (txIDmap[txID][1] == node)
   else if (txIDmap[txID][1] == node)
      FRMap[txID][1] = true;

   if (FRMap[txID][0] == true && FRMap[txID][1] == true){
      uint32_t node1 = txIDmap[txID][0];
      uint32_t node2 = txIDmap[txID][1];
      
      nodes[node1]->sendHoldPacket(txID, amount/2);
      nodes[node2]->sendHoldPacket(txID, amount/2);
   }
}

void
BLANCPPSync::onHoldPacket( uint32_t node, uint32_t txID, bool received){
   //std::cout<<received<<"  "<<txID<<" reporting\n"; 

   uint32_t node1 = txIDmap[txID][1];
   nodes[node1]->sendPayPacket(txID);
}

void
BLANCPPSync::onPayPacket( uint32_t node, uint32_t txID){
  if(txIDmap[txID][0] == node){
     uint32_t node1 = txIDmap[txID][0];
     uint32_t node2 = txIDmap[txID][1];

     nodes[node1]->reset(txID);
     nodes[node2]->reset(txID);

     //std::cout<<"All finished\n\n\n";
  } 
}

void
BLANCPPSync::onTxFail(uint32_t txID){
   uint32_t node1 = txIDmap[txID][0];
   uint32_t node2 = txIDmap[txID][1];

   nodes[node1]->reset(txID);
   nodes[node2]->reset(txID);}
}
}
