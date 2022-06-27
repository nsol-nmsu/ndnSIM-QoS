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

#include "tokenBucketDriver.hpp"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/integer.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "../NFD/daemon/fw/ndn-token-bucket.hpp"
#include "ns3/double.h"
#include "ns3/simulator.h"
#include "../helper/ndn-scenario-helper.hpp"
#include "TBucketRef.cpp"

#include "model/ndn-l3-protocol.hpp"
#include "helper/ndn-fib-helper.hpp"

#include <memory>

NS_LOG_COMPONENT_DEFINE( "ndn.TBDriver" );

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED( TBDriver );

TypeId
TBDriver::GetTypeId( void )
{
  static TypeId tid =
    TypeId( "ns3::ndn::TokenBucketDriver" )
      .SetGroupName( "Ndn" )
      .SetParent<App>()
      .AddConstructor<TBDriver>()
      .AddAttribute( "MaxBuckets", "Capacity of token bucket", StringValue( "4" ),
                    MakeDoubleAccessor( &TBDriver::m_MaxBkts ),  MakeIntegerChecker<int32_t>() )
      .AddAttribute( "FillRates", "The fillrates given in order, fom high to low priortiy.", StringValue( "1 1 1" ),
                    MakeStringAccessor( &TBDriver::m_fillRates ), MakeStringChecker() )
      .AddAttribute( "Capacities", "The fillrates given in order, fom high to low priortiy.", StringValue( "80 80 80" ),
                    MakeStringAccessor( &TBDriver::m_capacities ), MakeStringChecker() );

  return tid;
}

TBDriver::TBDriver()
  :m_connected( false )
{
    NS_LOG_FUNCTION_NOARGS();
}

// Inherited from Application base class.
void
TBDriver::StartApplication()
{

  NS_LOG_FUNCTION_NOARGS();
  App::StartApplication();
  nfd::fw::CT.drivers[ns3::NodeContainer::GetGlobal().Get( ns3::Simulator::GetContext())->GetId()] = this;
}

void
TBDriver::StopApplication()
{
  NS_LOG_FUNCTION_NOARGS();
  App::StopApplication();
}

void
TBDriver::ScheduleNextToken( int bucket )
{
  ns3::Ptr<ns3::Node> node= ns3::NodeContainer::GetGlobal().Get( ns3::Simulator::GetContext() );

  bool first;
  double fillRate;

  first = m_firsts[bucket];
  fillRate =  std::stod(SplitString(m_fillRates, ' ')[bucket]); 

  if ( first ) {
    m_sendEvent = Simulator::Schedule( Seconds( 0.0 ), &TBDriver::UpdateBucket, this, bucket );
  } 
  else {
    m_sendEvent = Simulator::Schedule( Seconds( 1.0 /fillRate ),
        &TBDriver::UpdateBucket, this, bucket );
  }
}

void
TBDriver::UpdateBucket( int bucket )
{ 
  bool first;
  double capacity;

  nfd::fw::TokenBucket* sender;

  int node= ns3::NodeContainer::GetGlobal().Get( ns3::Simulator::GetContext() )->GetId();
  bool paused = false;
  if(isSet()){
     if ( m_connected == false) {
        m_connected = true;
     }

     first = m_firsts[bucket]; 
     m_firsts[bucket] = false;
     capacity = std::stod(SplitString(m_capacities, ' ')[bucket]);
     sender = tbs[bucket];

    if(m_connected && !m_tokenFilled[bucket]){
      sender->noLongerAtCapacity.connect( [this, bucket]() {
        this->ScheduleNextToken(bucket);
      } );
      m_tokenFilled[bucket] = true;
    }

     // Check to make sure tokens are not generated beyong specified capacity
     if ( m_connected == true ) {
        sender->m_capacity = capacity;
        sender->addToken();
        paused = sender->atCapacity();

     }
  }  
  if(!paused)
     ScheduleNextToken( bucket );
}

void
TBDriver::addTokenBucket(nfd::fw::TokenBucket* tb){
  if(m_bktsSet>=m_bkts) return;
  
  tbs.push_back(tb);
  m_firsts.push_back(true);
  m_tokenFilled.push_back(false);
  ScheduleNextToken( m_bktsSet );  
  m_bktsSet++;
}


} // namespace ndn
} // namespace ns3
