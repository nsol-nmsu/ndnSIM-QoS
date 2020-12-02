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

#include "token-bucket.hpp"
#include "ns3/log.h"
#include "ns3/string.h"
#include "ns3/uinteger.h"
#include "ns3/packet.h"
#include "ns3/simulator.h"
#include "../NFD/daemon/fw/ndn-token-bucket.hpp"
#include "ConsumedTokens.hpp"
#include "ns3/double.h"
#include "ns3/simulator.h"
#include "../helper/ndn-scenario-helper.hpp"


#include "model/ndn-l3-protocol.hpp"
#include "helper/ndn-fib-helper.hpp"

#include <memory>

NS_LOG_COMPONENT_DEFINE( "ndn.TBucket" );

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED( TBucket );

TypeId
TBucket::GetTypeId( void )
{
  static TypeId tid =
    TypeId( "ns3::ndn::TokenBucket" )
      .SetGroupName( "Ndn" )
      .SetParent<App>()
      .AddConstructor<TBucket>()
      .AddAttribute( "FillRate1", "Fill rate of token bucket", StringValue( "1.0" ),
                    MakeDoubleAccessor( &TBucket::m_fillRate1 ), MakeDoubleChecker<double>() )
      .AddAttribute( "Capacity1", "Capacity of token bucket", StringValue( "80" ),
                    MakeDoubleAccessor( &TBucket::m_capacity1 ), MakeDoubleChecker<double>() )
      .AddAttribute( "FillRate2", "Fill rate of token bucket", StringValue( "1.0" ),
                    MakeDoubleAccessor( &TBucket::m_fillRate2 ), MakeDoubleChecker<double>() )
      .AddAttribute( "Capacity2", "Capacity of token bucket", StringValue( "80" ),
                    MakeDoubleAccessor( &TBucket::m_capacity2 ), MakeDoubleChecker<double>() )
      .AddAttribute( "FillRate3", "Fill rate of token bucket", StringValue( "1.0" ),
                    MakeDoubleAccessor( &TBucket::m_fillRate3 ), MakeDoubleChecker<double>() )
      .AddAttribute( "Capacity3", "Capacity of token bucket", StringValue( "80" ),
                    MakeDoubleAccessor( &TBucket::m_capacity3 ), MakeDoubleChecker<double>() );

  return tid;
}

TBucket::TBucket()
  :m_first1( true ),
   m_first2( true ),
   m_first3( true ),
   m_connected( false )
{
    NS_LOG_FUNCTION_NOARGS();
}

// Inherited from Application base class.
void
TBucket::StartApplication()
{
  NS_LOG_FUNCTION_NOARGS();
  App::StartApplication();

  ScheduleNextToken( 0 );
  ScheduleNextToken( 1 );
  ScheduleNextToken( 2 );
}

void
TBucket::StopApplication()
{
  NS_LOG_FUNCTION_NOARGS();
  App::StopApplication();
}

void
TBucket::ScheduleNextToken( int bucket )
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
    m_sendEvent = Simulator::Schedule( Seconds( 0.0 ), &TBucket::UpdateBucket, this, bucket );
  } else {
    m_sendEvent = Simulator::Schedule( Seconds( 1.0 /fillRate ),
        &TBucket::UpdateBucket, this, bucket );
  }
}

void
TBucket::UpdateBucket( int bucket )
{ 
  bool first;
  double capacity;
  nfd::fw::TokenBucket* sender;

  int node= ns3::NodeContainer::GetGlobal().Get( ns3::Simulator::GetContext() )->GetId();

  if ( m_connected == false && nfd::fw::CT.hasSender[node] == true ) {
    m_connected = true;
  }

  if ( bucket == 0 ) { 
    first = m_first1; 
    m_first1 = false;
    capacity = m_capacity1;
    sender = nfd::fw::CT.sender1[node];
  } else if ( bucket == 1 ) { 
    first = m_first2; 
    m_first2 = false;
    capacity = m_capacity2;
    sender = nfd::fw::CT.sender2[node];
  } else {
    first = m_first3; 
    m_first3 = false;
    capacity = m_capacity3;
    sender = nfd::fw::CT.sender3[node];
  }

  // Check to make sure tokens are not generated beyong specified capacity
  if ( m_connected == true ) {
    sender->m_capacity = capacity;
    sender->addToken();
  }

  ScheduleNextToken( bucket );
}

} // namespace ndn
} // namespace ns3
