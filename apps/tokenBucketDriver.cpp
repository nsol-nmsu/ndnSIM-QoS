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
      .AddAttribute( "FillRate1", "Fill rate of token bucket", StringValue( "1.0" ),
                    MakeDoubleAccessor( &TBDriver::m_fillRate1 ), MakeDoubleChecker<double>() )
      .AddAttribute( "Capacity1", "Capacity of token bucket", StringValue( "80" ),
                    MakeDoubleAccessor( &TBDriver::m_capacity1 ), MakeDoubleChecker<double>() )
      .AddAttribute( "FillRate2", "Fill rate of token bucket", StringValue( "1.0" ),
                    MakeDoubleAccessor( &TBDriver::m_fillRate2 ), MakeDoubleChecker<double>() )
      .AddAttribute( "Capacity2", "Capacity of token bucket", StringValue( "80" ),
                    MakeDoubleAccessor( &TBDriver::m_capacity2 ), MakeDoubleChecker<double>() )
      .AddAttribute( "FillRate3", "Fill rate of token bucket", StringValue( "1.0" ),
                    MakeDoubleAccessor( &TBDriver::m_fillRate3 ), MakeDoubleChecker<double>() )
      .AddAttribute( "Capacity3", "Capacity of token bucket", StringValue( "80" ),
                    MakeDoubleAccessor( &TBDriver::m_capacity3 ), MakeDoubleChecker<double>() );

  return tid;
}

TBDriver::TBDriver()
  :m_first1( true ),
   m_first2( true ),
   m_first3( true ),
   m_tokenFilledCB1(false),
   m_tokenFilledCB2(false),
   m_tokenFilledCB3(false),
   m_connected( false )
{
    NS_LOG_FUNCTION_NOARGS();
}

// Inherited from Application base class.
void
TBDriver::StartApplication()
{
  NS_LOG_FUNCTION_NOARGS();
  App::StartApplication();

  ScheduleNextToken( 0 );
  ScheduleNextToken( 1 );
  ScheduleNextToken( 2 );
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
    m_sendEvent = Simulator::Schedule( Seconds( 0.0 ), &TBDriver::UpdateBucket, this, bucket );
  } else {
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

  if ( m_connected == false && nfd::fw::CT.hasSender[node] == true ) {
    m_connected = true;
  }

  if ( bucket == 0 ) { 
    first = m_first1; 
    m_first1 = false;

    capacity = m_capacity1;
    sender = nfd::fw::CT.sender1[node];

    if(m_connected && !m_tokenFilledCB1){
        sender->noLongerAtCapacity.connect( [this]() {
           this->ScheduleNextToken(0);
        } );
        m_tokenFilledCB1 = true;
    }
  } else if ( bucket == 1 ) { 
    first = m_first2; 
    m_first2 = false;

    capacity = m_capacity2;
    sender = nfd::fw::CT.sender2[node];

    if(m_connected && !m_tokenFilledCB2){
        sender->noLongerAtCapacity.connect( [this]() {
           this->ScheduleNextToken(1);
        } );
        m_tokenFilledCB2 = true;
    }
  } else {
    first = m_first3; 
    m_first3 = false;

    capacity = m_capacity3;
    sender = nfd::fw::CT.sender3[node];

    if(m_connected && !m_tokenFilledCB3){
        sender->noLongerAtCapacity.connect( [this]() {
           this->ScheduleNextToken(2);
        } );
        m_tokenFilledCB3 = true;
    }
  }
  
  bool paused = false;
  // Check to make sure tokens are not generated beyong specified capacity
  if ( m_connected == true ) {

    sender->m_capacity = capacity;
    sender->addToken();
    paused = sender->atCapacity();
  }
  
  if(!paused)
     ScheduleNextToken( bucket );
}

} // namespace ndn
} // namespace ns3
