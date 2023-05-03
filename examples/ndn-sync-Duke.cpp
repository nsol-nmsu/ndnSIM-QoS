/* -*- Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil -*- */
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

#include "ndn-sync-Duke.hpp"
namespace ns3 {

int
main( int argc, char* argv[] )
{
  int run = 0;	
  bool DDoS = false, mit = false, icaap = true;
  std::string fillrates = "400 500 1200 200", capacities = "200 200 500 200";
  std::string config = "../topology/interface/caseDukeV3.txt";
  std::string device = "../topology/interface/dataDuke.txt";
  double timeStep = 1.0, endTime = 10800.0;

  // Read optional command-line parameters ( e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;

  cmd.AddValue("Run", "Run", run);
  cmd.AddValue("DDoS", "DDoS", DDoS);
  cmd.AddValue("Mitigation", "Mitigation", mit);
  cmd.AddValue("ICAAP", "ICAAP", icaap);
  cmd.AddValue("Fillrates", "Fillrates", fillrates);
  cmd.AddValue("Capacities", "Capacities", capacities);
  cmd.AddValue("ConfigFile", "ConfigFile", config);
  cmd.AddValue("DeviceFile", "DeviceFile", device);
  cmd.AddValue("TimeStep", "TimeStep", timeStep);
  cmd.AddValue("EndTime", "EndTime", endTime);


  cmd.Parse( argc, argv );

  sync.setTimeStep( timeStep );                // Each time step is 5 seconds.

  // Open the configuration file for reading
  ifstream configFile ( config, std::ios::in );
  ifstream deviceFile ( device, std::ios::in );       // Device - Node mapping

  json jf = json::parse( deviceFile );
  json::iterator it = jf.begin();

  unordered_map<int,std::vector<std::string>> nameMap;

  // Debug print
  while ( it !=  jf.end() )
  {
     nameMap[ it.value() ].push_back( it.key() );
     it++;
  }

  sync.fillNameMap( jf );
  
  std::string strLine;
  bool gettingNodeCount = false, buildingNetworkTopo = false, attachingWACs = false;
  bool attachingPMUs = false, attachingLCs = false, TypeI = false, TypeII = false;
  bool failLinks = false, injectData = false, Sync = false;
  std::vector<std::string> netParams;

  NodeContainer nodes;
  int nodeCount = 0;
  std::pair<int,int> flow_pair;
  unordered_map<int,int> used;
  unordered_map<int,int> usedS;
  unordered_map<int,int> usedR;


  // Setting default parameters for PointToPoint links and channels
  Config::SetDefault("ns3::QueueBase::MaxSize", StringValue("300p"));

  PointToPointHelper p2p;
  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;
  std::string strcallback;

  ndn::AppHelper producerHelper( "ns3::ndn::QoSProducer" );
  ndn::AppHelper consumerHelper( "ns3::ndn::QoSConsumer" );
  ndn::AppHelper syncHelper( "ns3::ndn::ConsumerQos");

  flowfile.open( "ndn_all_flows.csv", std::ios::out );
  srand( run );

  if ( configFile.is_open () ) {

    while ( std::getline( configFile, strLine ) ) {

      // Determine what operation is ongoing while reading the config file
      if( strLine.substr( 0,7 ) == "BEG_000" ) { gettingNodeCount = true; continue; }
      if( strLine.substr( 0,7 ) == "END_000" ) {
        // Create nodes
        nodeCount++; // Counter increment needed because nodes start from 1 not 0
        gettingNodeCount = false;
        nodes.Create( nodeCount );

        continue; 
      }
      if( strLine.substr( 0,7 ) == "BEG_001" ) { buildingNetworkTopo = true; continue; }
      if( strLine.substr( 0,7 ) == "END_001" ) { buildingNetworkTopo = false;
        attachingLCs = false; 
        ndn::StackHelper ndnHelper; 
        ndnHelper.InstallAll();

        ndnGlobalRoutingHelper.Install( nodes );

        continue; 
      }
      if( strLine.substr( 0,7 ) == "BEG_505" ) { Sync = true; continue; }
      if( strLine.substr( 0,7 ) == "END_505" ) { Sync = false; uniqueLCs.clear(); continue; }      
      if( strLine.substr( 0,7 ) == "BEG_005" ) { continue; }
      if( strLine.substr( 0,7 ) == "END_005" ) { TypeI = false; uniquePMUs.clear(); continue; }
      if( strLine.substr( 0,7 ) == "BEG_006" ) { //TypeII = true; 
	      continue; }
      if( strLine.substr( 0,7 ) == "END_006" ) { TypeII = false; uniqueWACs.clear();  uniquePMUs.clear(); continue;}
      if( strLine.substr( 0,7 ) == "BEG_100" ) { failLinks = true; continue; }
      if( strLine.substr( 0,7 ) == "END_100" ) { failLinks = false; continue; }
      if( strLine.substr( 0,7 ) == "BEG_101" ) { injectData = true; continue; }
      if( strLine.substr( 0,7 ) == "END_101" ) { injectData = false; continue; }


      if( gettingNodeCount == true ) {
        //Getting number of nodes to create
        netParams = SplitString( strLine );
        nodeCount = stoi( netParams[1] );

        //Store node ID ranges for various node types
      } 
      else if( buildingNetworkTopo == true ) {

        // Building network topology
        netParams = SplitString( strLine );
        int mbps = std::stoi(netParams[2])*10;
        p2p.SetDeviceAttribute( "DataRate", StringValue( std::to_string(mbps)+"Mbps" ) );
        p2p.SetChannelAttribute( "Delay", StringValue( netParams[3]+"ms" ) );
        p2p.Install( nodes.Get( std::stoi( netParams[0] ) ), nodes.Get( std::stoi( netParams[1] ) ) );
      } 
      else if( attachingWACs == true ) {

        // Attaching WACs to routers
        netParams = SplitString( strLine );
        p2p.SetDeviceAttribute( "DataRate", StringValue( netParams[2]+"Mbps" ) );
        p2p.SetChannelAttribute( "Delay", StringValue( netParams[3]+"ms" ) );
        p2p.Install( nodes.Get( std::stoi( netParams[0] ) ), nodes.Get( std::stoi( netParams[1] ) ) );
      } 
      else if( attachingPMUs == true ) {

        // Attaching PMUs to routers
        netParams = SplitString( strLine );
        p2p.SetDeviceAttribute( "DataRate", StringValue( netParams[2]+"Mbps" ) );
        p2p.SetChannelAttribute( "Delay", StringValue( netParams[3]+"ms" ) );
        p2p.Install( nodes.Get( std::stoi( netParams[0] ) ), nodes.Get( std::stoi( netParams[1] ) ) );
      } 
      else if( attachingLCs == true ) {

        // Attaching LCs to routers
        netParams = SplitString( strLine );
        p2p.SetDeviceAttribute( "DataRate", StringValue( netParams[2]+"Mbps" ) );
        p2p.SetChannelAttribute( "Delay", StringValue( netParams[3]+"ms" ) );
        p2p.Install( nodes.Get( std::stoi( netParams[0] ) ), nodes.Get( std::stoi( netParams[1] ) ) );
      } 
      else if(Sync == true) {
         // Install apps on LCs and PMUs for data exchange
         netParams = SplitString( strLine );

         // Install app on unique LC WACs  
	 std::cout<<strLine<<std::endl;
	 if ( IsLCAppInstalled( netParams[0] ) == false ) {

 	    char temp[10];
	    sprintf( temp, "%lf", 1.0/( float( rand()%40+80 ) ) );
	    if ( DEBUG) std::cout << netParams[1] << "\n";

	    // Install consumer
	    syncHelper.SetPrefix( "/power/typeII" );
	    syncHelper.SetAttribute( "Subscription", IntegerValue( 0 ) );
	    syncHelper.SetAttribute( "PayloadSize", StringValue( "200" ) );
	    syncHelper.SetAttribute( "RetransmitPackets", IntegerValue( 0 ) );
	    syncHelper.SetAttribute( "LifeTime", StringValue( "1s" ) );
	    syncHelper.Install( nodes.Get( std::stoi( netParams[0] ) ) );
	    usedS[std::stoi( netParams[0] )] = 1;

	    auto apps = nodes.Get( std::stoi( netParams[0] ) )->GetApplication( 0 )->GetObject<ns3::ndn::ConsumerQos>();
	    sync.addSender( std::stoi( netParams[0] ),apps );

	    // Install producer
	    producerHelper.SetPrefix( "/power/typeI/data" );
	    producerHelper.SetAttribute( "Frequency", StringValue( "0" ) );
	    producerHelper.Install( nodes.Get( std::stoi( netParams[0] ) ) );
	    producerHelper.SetPrefix( "/power/typeII/data" );
	    producerHelper.SetAttribute( "Frequency", StringValue( "0" ) );
	    producerHelper.Install( nodes.Get( std::stoi( netParams[0] ) ) );
	    used[std::stoi( netParams[0] )] = 1;
	    producerHelper.SetPrefix( "/power/typeIII/data" );
	    producerHelper.SetAttribute( "Frequency", StringValue( "0" ) );
	    producerHelper.Install( nodes.Get( std::stoi( netParams[0] ) ) );
	    used[std::stoi( netParams[0] )] = 1;
	    //usedS[std::stoi( netParams[0] )] = 1;

	    // Setup node to originate prefixes for dynamic routing
	    ndnGlobalRoutingHelper.AddOrigin( "/power/typeI/data", nodes.Get( std::stoi( netParams[0] ) ) );
	    ndnGlobalRoutingHelper.AddOrigin( "/power/typeII/data", nodes.Get( std::stoi( netParams[0] ) ) );
	    ndnGlobalRoutingHelper.AddOrigin( "/power/typeIII/data", nodes.Get( std::stoi( netParams[0] ) ) );

	    usedR[std::stoi( netParams[0] )] = 1;
	    sync.setPVNode(std::stoi( netParams[0] ));
	 }

	 // Install flow app on PMUs to send data to LCs
	 if ( IsPMUAppInstalled( netParams[1] ) == false ){
  	    char temp[10];
	    sprintf( temp, "%lf", 1.0/( float( rand()%40+80 ) ) );
	    std::string pre = "";
	    for(int i = 0; i < nameMap[std::stoi( netParams[1] )].size(); i++ ) {
 	       std::size_t found = nameMap[std::stoi( netParams[1] )][i].find("SW");
	       std::string com_string = nameMap[std::stoi( netParams[1] )][i].substr(0,1);
	       if (found!=std::string::npos || com_string == "V" || com_string == "R" ){
 	          pre = "/power/typeI";
		  continue;
	       }
	       if( ( com_string == "B" || com_string == "P") && pre != "/power/typeI" )            
	          pre = "/power/typeII";
	       if( (com_string == "C" || com_string == "L" || com_string == "T") && (pre != "/power/typeI" && pre != "/power/typeII"))
                  pre = "/power/typeIII";
	    }
	    if(pre == "") pre = "/power/typeII";

	    syncHelper.SetPrefix( pre );
	    syncHelper.SetAttribute( "LeadPrefix", StringValue("/power/typeII") );
	    syncHelper.SetAttribute( "Subscription", IntegerValue( 0 ) );
	    syncHelper.SetAttribute( "PayloadSize", StringValue( "200" ) );
	    syncHelper.SetAttribute( "RetransmitPackets", IntegerValue( 0 ) );
	    syncHelper.SetAttribute( "LifeTime", StringValue( "1000ms" ) );

	    syncHelper.Install( nodes.Get( std::stoi( netParams[1] ) ) );
	    used[std::stoi( netParams[1] )] = 1;
	    //Config::ConnectWithoutContext( strcallback, MakeCallback( &SentInterestCallbackPhy ) );
	    usedS[std::stoi( netParams[1] )] = 1;
	    //std::cout << "installed on " << netParams[1] <<std::endl;
	    auto apps = nodes.Get( std::stoi( netParams[1] ) )->GetApplication( 0 )->GetObject<ns3::ndn::ConsumerQos>();

	    sync.addSender( std::stoi( netParams[1] ),apps );

	    // Setup node to originate prefixes for dynamic routing
	    for(int i = 0; i < nameMap[std::stoi( netParams[1] )].size(); i++ ) {
	       producerHelper.SetPrefix( "/power/typeI/phy"+ netParams[1] );
	       producerHelper.SetAttribute( "Frequency", StringValue( "0" ) );
	       producerHelper.Install( nodes.Get( std::stoi( netParams[1] ) ) );

	       producerHelper.SetPrefix( "/power/typeII/phy"+ netParams[1] );
	       producerHelper.SetAttribute( "Frequency", StringValue( "0" ) );
	       producerHelper.Install( nodes.Get( std::stoi( netParams[1] ) ) );

	       if ( DEBUG) std::cout << "Adding origin " << "/power/typeI/phy" + netParams[1] + "/" + nameMap[std::stoi( netParams[1] )][i] << " at node " << netParams[1] << std::endl;

	       std::string com_string = nameMap[std::stoi( netParams[1] )][i].substr(0,2);

	       if(com_string == "BE" || com_string == "PV"){
	          std::cout<<"/power/typeI/phy"<< netParams[1]<<"/"<<nameMap[std::stoi( netParams[1] )][i]<<std::endl;
		  ndnGlobalRoutingHelper.AddOrigin( "/power/typeI/phy" + netParams[1] + "/" +
  		  		  nameMap[std::stoi( netParams[1] )][i], nodes.Get( std::stoi( netParams[1] ) ) );
		  ndnGlobalRoutingHelper.AddOrigin( "/power/typeII/phy" + netParams[1] + "/" +
  		  		  nameMap[std::stoi( netParams[1] )][i], nodes.Get( std::stoi( netParams[1] ) ) );
	       }
	       usedR[std::stoi( netParams[1] )] = 2;
	    }

	    //ndnGlobalRoutingHelper.AddOrigin( "/power/typeII/data", nodes.Get( std::stoi( netParams[1] ) ) );
	    //usedR[std::stoi( netParams[1] )] = 1;

	 }

	 // Save the flow
	 flow_pair.first = stoi( netParams[0] );
	 flow_pair.second = stoi( netParams[1] );
	 all_flows.push_back( flow_pair );

	 // Write flow to file
	 flowfile << netParams[0] << " " << netParams[1] << " TypeI" << std::endl;
	 flowfile << netParams[0] << " " << netParams[1] << " TypeII" << std::endl;
	 flowfile << netParams[0] << " " << netParams[1] << " TypeIII" << std::endl;
      }
      else if(injectData == true && DDoS) {
        netParams = SplitString( strLine );
	      char temp[10];
        sprintf( temp, "%lf", 1.0/( float( rand()%50+1000 ) ) );        
        consumerHelper.SetPrefix( "/pow/typeI/back/dat"+netParams[0]+"/phy" + netParams[1] );
        consumerHelper.SetAttribute( "Frequency", StringValue( temp ) ); //0.016 or 0.02
        consumerHelper.SetAttribute( "Subscription", IntegerValue( 0 ) );
        consumerHelper.SetAttribute( "PayloadSize", StringValue( "10240" ) );
        consumerHelper.SetAttribute( "RetransmitPackets", IntegerValue( 0 ) );
        consumerHelper.SetAttribute( "Offset", IntegerValue( 0 ) );
        consumerHelper.SetAttribute( "LifeTime", StringValue( "1s" ) );	 
        consumerHelper.Install( nodes.Get( std::stoi( netParams[0] ) ) ); 
        consumerHelper.SetPrefix( "/pow/typeII/back/dat"+netParams[0]+"/phy" + netParams[1] );
        consumerHelper.Install( nodes.Get( std::stoi( netParams[0] ) ) );
        consumerHelper.SetPrefix( "/pow/typeIII/back/dat"+netParams[0]+"/phy" + netParams[1] );
        consumerHelper.Install( nodes.Get( std::stoi( netParams[0] ) ) );
        
        ndnGlobalRoutingHelper.AddOrigin( "/pow/typeI", nodes.Get( std::stoi( netParams[1] ) ) );
        ndnGlobalRoutingHelper.AddOrigin( "/pow/typeII", nodes.Get( std::stoi( netParams[1] ) ) );
        ndnGlobalRoutingHelper.AddOrigin( "/pow/typeIII", nodes.Get( std::stoi( netParams[1] ) ) );
        
	      used[std::stoi( netParams[0] )] = 1;
      } 
      else {
        //std::cout << "reading something else " << strLine << std::endl;
      }	

    } //end while
  } 
  else {
    std::cout << "Cannot open configuration file!!!" << std::endl;
    exit( 1 );
  }

  configFile.close();
  std::cout<<"All done\n";

  // Define the routing strategies per prefix
  if (!icaap){
    ndn::StrategyChoiceHelper::InstallAll( "/power/typeI", "/localhost/nfd/strategy/multicast" );
    ndn::StrategyChoiceHelper::InstallAll( "/power/typeII", "/localhost/nfd/strategy/multicast" );
    ndn::StrategyChoiceHelper::InstallAll( "/power/typeIII", "/localhost/nfd/strategy/best-route" );
    ndn::StrategyChoiceHelper::InstallAll( "/pow/typeI", "/localhost/nfd/strategy/multicast" );
    ndn::StrategyChoiceHelper::InstallAll( "/pow/typeII", "/localhost/nfd/strategy/multicast" );
    ndn::StrategyChoiceHelper::InstallAll( "/pow/typeIII", "/localhost/nfd/strategy/best-route" );
  }
  else {
    ndn::AppHelper tokenHelper( "ns3::ndn::TokenBucketDriver" );
    tokenHelper.SetAttribute( "FillRates", StringValue( fillrates ) ); 
    tokenHelper.SetAttribute( "Capacities", StringValue( capacities ) ); 
    
    for ( int i=0; i<( int )nodes.size(); i++ ) {
      if( used[i] == 0 ){
        ndn::StrategyChoiceHelper::Install( nodes.Get( i ),"power/typeI", "/localhost/nfd/strategy/qos" );
        ndn::StrategyChoiceHelper::Install( nodes.Get( i ),"power/typeII", "/localhost/nfd/strategy/qos" );
        ndn::StrategyChoiceHelper::Install( nodes.Get( i ),"power/typeIII", "/localhost/nfd/strategy/qos" );
        ndn::StrategyChoiceHelper::Install( nodes.Get( i ),"pow/typeI", "/localhost/nfd/strategy/qos" );
        ndn::StrategyChoiceHelper::Install( nodes.Get( i ),"pow/typeII", "/localhost/nfd/strategy/qos" );
        ndn::StrategyChoiceHelper::Install( nodes.Get( i ),"pow/typeIII", "/localhost/nfd/strategy/qos" );

        tokenHelper.Install( nodes.Get( i ) );
      }
    }
  }

  // Populate routing table for nodes
  ndn::GlobalRoutingHelper::CalculateAllPossibleRoutes();
	  
  sync.beginSync();

  std::cout<<"Routes calculated\n";

  //Open trace file for writing
  char trace[100];
  if(icaap)
     sprintf( trace, "ndn-case_icaap-run%d.csv", run );
  else 
     sprintf( trace, "ndn-case_base-run%d.csv", run);

  tracefile.open( trace, std::ios::out );
  tracefile << "nodeid, event, name, payloadsize, time" << std::endl;
  for ( int i=0; i<( int )nodes.size(); i++ ) {
    if( usedS[i] == 1 ) {
      strcallback = "/NodeList/" + std::to_string( i ) + "/ApplicationList/" + "*/SentInterest";
      Config::ConnectWithoutContext( strcallback, MakeCallback( &SentInterestCallbackPhy ) );
    }
    if( usedR[i] == 1 ) {
      strcallback = "/NodeList/" + std::to_string( i ) + "/ApplicationList/" + "*/ReceivedInterest";
      Config::ConnectWithoutContext( strcallback, MakeCallback( &ReceivedInterestCallbackCom ) );
    }
    else if (usedR[i] == 2) {
		  strcallback = "/NodeList/" + std::to_string( i ) + "/ApplicationList/" + "*/ReceivedInterest";
		  Config::ConnectWithoutContext( strcallback, MakeCallback( &ReceivedInterestCallbackPhy ) );
		}		 
  }

  Simulator::Stop( Seconds( endTime ) );
  Simulator::Run();
  sync.sendSync();
  Simulator::Destroy();

  return 0;
}

}
int
main( int argc, char* argv[] )
{
  return ns3::main( argc, argv );
}
