/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright ( c ) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or ( at your option ) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/
#include "ndn-subscriber-sync.hpp"
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

#include "utils/ndn-ns3-packet-tag.hpp"
#include "utils/ndn-rtt-mean-deviation.hpp"

#include <ndn-cxx/lp/tags.hpp>

#include <boost/lexical_cast.hpp>
#include <boost/ref.hpp>

#include <fstream>
using json = nlohmann::json;

#define DEBUG 0

NS_LOG_COMPONENT_DEFINE( "ndn.ConsumerQos" );

namespace ns3 {
namespace ndn {

NS_OBJECT_ENSURE_REGISTERED( ConsumerQos );

TypeId
ConsumerQos::GetTypeId( void )
{
  static TypeId tid =
    TypeId( "ns3::ndn::ConsumerQos" )
      .SetGroupName( "Ndn" )
      .SetParent<App>()
      .AddConstructor<ConsumerQos>()

      .AddAttribute( "StartSeq", "Initial sequence number", IntegerValue( 0 ),
                    MakeIntegerAccessor( &ConsumerQos::m_seq ), MakeIntegerChecker<int32_t>() )
      .AddAttribute( "Prefix", "Name of the Interest", StringValue( "/" ),
                    MakeNameAccessor( &ConsumerQos::m_interestName ), MakeNameChecker() )
      .AddAttribute( "LifeTime", "LifeTime for subscription packet", StringValue( "5400s" ),
                    MakeTimeAccessor( &ConsumerQos::m_interestLifeTime ), MakeTimeChecker() )
      .AddAttribute( "Frequency",
                    "Timeout defining how frequently subscription should be reinforced",
		    TimeValue( Seconds( 60 ) ),
                    MakeTimeAccessor( &ConsumerQos::m_txInterval ), MakeTimeChecker() )

      .AddAttribute( "RetxTimer",
                    "Timeout defining how frequent retransmission timeouts should be checked",
                    StringValue( "50s" ),
                    MakeTimeAccessor( &ConsumerQos::GetRetxTimer, &ConsumerQos::SetRetxTimer ),
                    MakeTimeChecker() )

      .AddAttribute( "RetransmitPackets", "Retransmit lost packets if set to 1, otherwise do not perform retransmission", IntegerValue( 1 ),
                    MakeIntegerAccessor( &ConsumerQos::m_doRetransmission ), MakeIntegerChecker<int32_t>() )

      .AddAttribute( "Subscription", "Subscription value for the interest. 0-normal interest, 1-soft subscribe, 2-hard subscriber, 3-unsubsribe", IntegerValue( 2 ),
                    MakeIntegerAccessor( &ConsumerQos::m_subscription ), MakeIntegerChecker<int32_t>() )

      .AddAttribute( "Offset", "Random offset to randomize sending of interests", IntegerValue( 0 ),
                    MakeIntegerAccessor( &ConsumerQos::m_offset ), MakeIntegerChecker<int32_t>() )

      .AddAttribute( "PayloadSize", "Virtual payload size for interest packets", UintegerValue( 0 ),
                    MakeUintegerAccessor( &ConsumerQos::m_virtualPayloadSize ),
                    MakeUintegerChecker<uint32_t>() )

      .AddTraceSource( "LastRetransmittedInterestDataDelay",
                      "Delay between last retransmitted Interest and received Data",
                      MakeTraceSourceAccessor( &ConsumerQos::m_lastRetransmittedInterestDataDelay ),
                      "ns3::ndn::Subscriber::LastRetransmittedInterestDataDelayCallback" )

      .AddTraceSource( "FirstInterestDataDelay",
                      "Delay between first transmitted Interest and received Data",
                      MakeTraceSourceAccessor( &ConsumerQos::m_firstInterestDataDelay ),
                      "ns3::ndn::Subscriber::FirstInterestDataDelayCallback" )

      .AddTraceSource( "ReceivedData", "ReceivedData",
                      MakeTraceSourceAccessor( &ConsumerQos::m_receivedData ),
                      "ns3::ndn::Subscriber::ReceivedDataTraceCallback" )

      .AddTraceSource( "SentInterest", "SentInterest",
                      MakeTraceSourceAccessor( &ConsumerQos::m_sentInterest ),
                      "ns3::ndn::Subscriber::SentInterestTraceCallback" );
      ;

	  return tid;
}

ConsumerQos::ConsumerQos()
    : m_rand( CreateObject<UniformRandomVariable>() )
    , m_seq( 0 )
    , m_seqMax( std::numeric_limits<uint32_t>::max() ) // set to max value on uint32
    , m_firstTime ( true )
    , m_doRetransmission( 1 )
{
  NS_LOG_FUNCTION_NOARGS();

  m_rtt = CreateObject<RttMeanDeviation>();
}


ConsumerQos::~ConsumerQos()
{
}


void
ConsumerQos::SetRetxTimer( Time retxTimer )
{

	// Do not restranmit lost packets, if set to 0
	if( m_doRetransmission == 1 ) {

		m_retxTimer = retxTimer;

		if ( m_retxEvent.IsRunning() ) {

			//m_retxEvent.Cancel (); // Cancel any scheduled cleanup events
			Simulator::Remove( m_retxEvent ); // slower, but better for memory
		}

		// Schedule even with new timeout
		m_retxEvent = Simulator::Schedule( m_retxTimer, &ConsumerQos::CheckRetxTimeout, this );
	}
}


Time
ConsumerQos::GetRetxTimer() const
{
	return m_retxTimer;
}


void
ConsumerQos::CheckRetxTimeout()
{

	Time now = Simulator::Now();
	Time rto = m_rtt->RetransmitTimeout();

	// NS_LOG_DEBUG ( "Current RTO: " << rto.ToDouble ( Time::S ) << "s" );

	while ( !m_seqTimeouts.empty() ) {

		SeqTimeoutsContainer::index<i_timestamp>::type::iterator entry =
			m_seqTimeouts.get<i_timestamp>().begin();

		// timeout expired?
		if ( entry->time + rto <= now )	{

			uint32_t seqNo = entry->seq;
			m_seqTimeouts.get<i_timestamp>().erase( entry );
			OnTimeout( seqNo );

		} else {

			break; // nothing else to do. All later packets need not be retransmitted
		}
	}

	m_retxEvent = Simulator::Schedule( m_retxTimer, &ConsumerQos::CheckRetxTimeout, this );
}



// Application methods
void
ConsumerQos::StartApplication()
{
	NS_LOG_FUNCTION_NOARGS();
	App::StartApplication();
	//ScheduleNextPacket();
}

void
ConsumerQos::StopApplication() // Called at time specified by Stop
{
	NS_LOG_FUNCTION_NOARGS();
	Simulator::Cancel( m_sendEvent );
	App::StopApplication();
}

void
ConsumerQos::SendPacket( std::string deviceName, std::string payload, bool agg )
{
	shared_ptr<Name> DName = make_shared<Name>( deviceName );
	m_payloads[DName->getSubName( -1, 1 ).toUri()] = payload;

	// Set default size for payload interets
	if ( m_subscription == 0 && m_virtualPayloadSize == 0 ) {

		m_virtualPayloadSize = 4;
	}

	//m_virtualPayloadSize = payloadSize;

	if ( !m_active ) {
		return;
	}

	std::string leadPayload = leadMeasurements.dump();

	if ( agg && leadPayload.length() > 10 ) {

		if ( DEBUG ) std::cout << "Let us see this \n\n" << leadPayload << "\n\n\n";
		payload = leadPayload;
	}

	NS_LOG_FUNCTION_NOARGS();

	uint32_t seq = std::numeric_limits<uint32_t>::max();

	while ( m_retxSeqs.size() ) {

		seq = *m_retxSeqs.begin();
		m_retxSeqs.erase( m_retxSeqs.begin() );
		break;

	}

	if ( m_seqMax != std::numeric_limits<uint32_t>::max() ) {

		if ( m_seq >= m_seqMax ) {

			return;
		}
	}

	seq = m_seq++;

	std::vector<uint8_t> myVector( payload.begin(), payload.end() );
	uint8_t *p = &myVector[0];
	//uint8_t payloa[1] = {1};
	shared_ptr<Name> nameWithSequence = make_shared<Name>( m_interestName.toUri() + "/" + deviceName );

	shared_ptr<Interest> interest = make_shared<Interest>();
	interest->setNonce( m_rand->GetValue( 0, std::numeric_limits<uint32_t>::max() ) );
	interest->setSubscription( m_subscription );
	//nameWithSequence->append( deviceName.c_str() );
	nameWithSequence->appendSequenceNumber( seq );

	if ( m_subscription == 0 ) {
		interest->setPayload( p, myVector.size()/*m_virtualPayloadSize*/ ); // Add payload to interest
	}

	interest->setName( *nameWithSequence );
	time::milliseconds interestLifeTime( m_interestLifeTime.GetMilliSeconds() );
	interest->setInterestLifetime( interestLifeTime );
	interest->setCanBePrefix(false);
	if ( DEBUG ) std::cout << "Sending...\n";
	if ( DEBUG ) std::cout << interest->getName() << " " << ns3::Simulator::GetContext() << " " << Simulator::Now() << std::endl;

	NS_LOG_INFO( "node( " << GetNode()->GetId() << " ) > sending Interest: " << interest->getName() /*m_interestName*/ << " with Payload = " << interest->getPayloadLength() << "bytes" );

	WillSendOutInterest( seq );

	m_transmittedInterests( interest, this, m_face );
	m_appLink->onReceiveInterest( *interest );

	// Callback for sent payload interests
	m_sentInterest( GetNode()->GetId(), interest );

	/*
	if ( packetsToSend == 0 ) {

		packetsToSend =2;
		ScheduleNextPacket( deviceName, payload, agg );
	}
	else {

		packetsToSend--;

		if ( packetsToSend != 0 ) {
			ScheduleNextPacket( deviceName, payload, agg );
		}
	}
	*/

}


void
ConsumerQos::ScheduleNextPacket(std::string deviceName, std::string payload, bool agg)
{

	//if ( m_firstTime ) {
	//	m_sendEvent = Simulator::Schedule( Seconds( double( m_offset ) ), &ConsumerQos::SendPacket, apps );
	//	m_firstTime = false;
	//} else
	if ( !m_sendEvent.IsRunning() ) {
		m_sendEvent = Simulator::Schedule( Seconds(0.1), &ConsumerQos::SendPacket, this, deviceName, payload, agg );
	}
}


///////////////////////////////////////////////////
//          Process incoming packets             //
///////////////////////////////////////////////////

void
ConsumerQos::OnData( shared_ptr<const Data> data )
{

	if ( !m_active ) {
		return;
	}

	App::OnData( data ); // tracing inside

	NS_LOG_FUNCTION( this << data );

	// This could be a problem......
	//uint32_t seq = data->getName().at( -1 ).toSequenceNumber();

	NS_LOG_INFO( "node( " << GetNode()->GetId() << " ) < Received DATA for " << data->getName() << " TIME: " << Simulator::Now() );

	// Callback for received subscription data
	m_receivedData( GetNode()->GetId(), data );

	int hopCount = 0;
	auto hopCountTag = data->getTag<lp::HopCountTag>();

	if ( hopCountTag != nullptr ) { // e.g., packet came from local node's cache
		hopCount = *hopCountTag;
	}

	NS_LOG_DEBUG( "Hop count: " << hopCount );

	//Enable trace file for Interests with sequence number ( subscription = 0 )
	if ( m_subscription == 0 ) {

		// This could be a problem......
		uint32_t seq = data->getName().at( -1 ).toSequenceNumber();

		SeqTimeoutsContainer::iterator entry = m_seqLastDelay.find( seq );

		if ( entry != m_seqLastDelay.end() ) {
			m_lastRetransmittedInterestDataDelay( this, seq, Simulator::Now() - entry->time, hopCount );
		}

		entry = m_seqFullDelay.find( seq );

		if ( entry != m_seqFullDelay.end() ) {
			m_firstInterestDataDelay( this, seq, Simulator::Now() - entry->time, m_seqRetxCounts[seq], hopCount );
		}

		m_seqRetxCounts.erase( seq );
		m_seqFullDelay.erase( seq );
		m_seqLastDelay.erase( seq );

		m_seqTimeouts.erase( seq );
		m_retxSeqs.erase( seq );

		m_rtt->AckSeq( SequenceNumber32( seq ) );
	}
}


void
ConsumerQos::OnTimeout( uint32_t sequenceNumber )
{
	//NS_LOG_FUNCTION( sequenceNumber );
	// std::cout << Simulator::Now () << ", TO: " << sequenceNumber << ", current RTO: " <<
	// m_rtt->RetransmitTimeout ().ToDouble ( Time::S ) << "s\n";

	/* 
	m_rtt->IncreaseMultiplier(); // Double the next RTO
	m_rtt->SentSeq( SequenceNumber32( sequenceNumber ),
			1 ); // make sure to disable RTT calculation for this sample
	m_retxSeqs.insert( sequenceNumber );
	*/
}


void
ConsumerQos::WillSendOutInterest( uint32_t sequenceNumber )
{
	NS_LOG_DEBUG( "Trying to add " << sequenceNumber << " with " << Simulator::Now() << ". already "
			<< m_seqTimeouts.size() << " items" );
	//m_seqTimeouts.insert( SeqTimeout( sequenceNumber, Simulator::Now() ) );
	m_seqFullDelay.insert( SeqTimeout( sequenceNumber, Simulator::Now() ) );
	m_seqLastDelay.erase( sequenceNumber );
	m_seqLastDelay.insert( SeqTimeout( sequenceNumber, Simulator::Now() ) );
	m_seqRetxCounts[sequenceNumber]++;
	m_rtt->SentSeq( SequenceNumber32( sequenceNumber ), 1 );

}


} // namespace ndn
} // namespace ns3