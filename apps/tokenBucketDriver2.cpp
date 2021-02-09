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

#include "tokenBucketDriver2.hpp"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "../NFD/daemon/fw/ndn-token-bucket.hpp"
#include "TBucketRef.hpp"
#include "ns3/double.h"
#include "ns3/simulator.h"
#include "../helper/ndn-scenario-helper.hpp"


#include "model/ndn-l3-protocol.hpp"
#include "helper/ndn-fib-helper.hpp"

#include <memory>

NS_LOG_COMPONENT_DEFINE( "ndn.TBDriver2" );

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED( TBDriver2 );

TypeId
TBDriver2::GetTypeId( void )
{
  static TypeId tid =
    TypeId( "ns3::ndn::TokenBucketDriver2" )
      .SetGroupName( "Ndn" )
      .SetParent<App>()
      .AddConstructor<TBDriver2>()
      .AddAttribute( "FillRate1", "Fill rate of token bucket", StringValue( "1.0" ),
                    MakeDoubleAccessor( &TBDriver2::m_fillRate1 ), MakeDoubleChecker<double>() )
      .AddAttribute( "Capacity1", "Capacity of token bucket", StringValue( "80" ),
                    MakeDoubleAccessor( &TBDriver2::m_capacity1 ), MakeDoubleChecker<double>() )
      .AddAttribute( "FillRate2", "Fill rate of token bucket", StringValue( "1.0" ),
                    MakeDoubleAccessor( &TBDriver2::m_fillRate2 ), MakeDoubleChecker<double>() )
      .AddAttribute( "Capacity2", "Capacity of token bucket", StringValue( "80" ),
                    MakeDoubleAccessor( &TBDriver2::m_capacity2 ), MakeDoubleChecker<double>() )
      .AddAttribute( "FillRate3", "Fill rate of token bucket", StringValue( "1.0" ),
                    MakeDoubleAccessor( &TBDriver2::m_fillRate3 ), MakeDoubleChecker<double>() )
      .AddAttribute( "Capacity3", "Capacity of token bucket", StringValue( "80" ),
                    MakeDoubleAccessor( &TBDriver2::m_capacity3 ), MakeDoubleChecker<double>() );

  return tid;
}

TBDriver2::TBDriver2()
  :m_first1( true ),
   m_first2( true ),
   m_first3( true ),
   m_connected( false )
{
    NS_LOG_FUNCTION_NOARGS();
}

// Inherited from Application base class.
void
TBDriver2::StartApplication()
{
  NS_LOG_FUNCTION_NOARGS();
  App::StartApplication();

  ScheduleNextToken( 0 );
  ScheduleNextToken( 1 );
  ScheduleNextToken( 2 );
}

void
TBDriver2::StopApplication()
{
  NS_LOG_FUNCTION_NOARGS();
  App::StopApplication();
}

void
TBDriver2::ScheduleNextToken( int bucket )
{
  ns3::Ptr<ns3::Node> node= ns3::NodeContainer::GetGlobal().Get( ns3::Simulator::GetContext() );

  bool first;
  double fillRate;

  if ( bucket == 0 ) {
    first = m_first1;
    fillRate =  m_fillRate1;
  } else if ( bucket == 1 ) {
    first = m_first2;
    fillRate =  m_fillRate2;
  } else {
    first = m_first3;
    fillRate =  m_fillRate3;
  }

  if ( first ) {
    m_sendEvent = Simulator::Schedule( Seconds( 0.0 ), &TBDriver2::UpdateBucket, this, bucket );
  } else {
    m_sendEvent = Simulator::Schedule( Seconds( 1.0 /fillRate ),
        &TBDriver2::UpdateBucket, this, bucket );
  }
}

void
TBDriver2::UpdateBucket( int bucket )
{ 
  bool first;
  double capacity;

  int node= ns3::NodeContainer::GetGlobal().Get( ns3::Simulator::GetContext() )->GetId();
  auto sender = nfd::fw::CT.sender1;
  auto connect_check = nfd::fw::CT.hasSender.begin();
  if ( m_connected == false && nfd::fw::CT.hasSender[node] == true ) {
    m_connected = true;
    while(connect_check != nfd::fw::CT.hasSender.end()){
       if(connect_check->second == false)
          m_connected = false;	       
       connect_check++;    
    }
  }

  if ( bucket == 0 ) { 
    first = m_first1; 
    m_first1 = false;
    capacity = m_capacity1;
    sender = nfd::fw::CT.sender1;
  } else if ( bucket == 1 ) { 
    first = m_first2; 
    m_first2 = false;
    capacity = m_capacity2;
    sender = nfd::fw::CT.sender2;
  } else {
    first = m_first3; 
    m_first3 = false;
    capacity = m_capacity3;
    sender = nfd::fw::CT.sender3;
  }

  // Check to make sure tokens are not generated beyond specified capacity
  if ( m_connected == true ) {
    auto TB = sender.begin();
    while(TB != sender.end()){ 
       TB->second->m_capacity = capacity;
       TB->second->addToken();
       TB++;
    }
  }

  ScheduleNextToken( bucket );
}

} // namespace ndn
} // namespace ns3
