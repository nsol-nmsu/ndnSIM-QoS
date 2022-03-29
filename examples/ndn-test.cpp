/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/ndnSIM-module.h"
#include "ns3/ndnSIM/helper/ndn-link-control-helper.hpp"
#include "ns3/ndnSIM/helper/ndn-fib-helper.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdlib.h>

#include <unordered_map>

#include "apps/ndn-app.hpp" //this header is required for Trace Sink

namespace ns3 {


void SentInterestCallbackPhy(uint32_t, shared_ptr<const ndn::Interest>);
void ReceivedInterestCallbackCom(uint32_t, shared_ptr<const ndn::Interest>);

void SentDataCallbackCom(uint32_t, shared_ptr<const ndn::Data>);
void ReceivedDataCallbackPhy(uint32_t, shared_ptr<const ndn::Data>);

void SentInterestCallbackAgg(uint32_t, shared_ptr<const ndn::Interest>);
void ReceivedInterestCallbackAgg(uint32_t, shared_ptr<const ndn::Interest>);

bool NodeInComm(int);
bool NodeInAgg(int);

// Split a line into a vector of strings
std::vector<std::string> SplitString(std::string);

//Get source node ID from which payload interest came from
uint32_t GetSourceNodeID (std::string name);

//Check if source/destination flow is permitted in config file (one-to-many, many-to-one flows)
bool FlowPermitted (int dstnode, int srcnode);

//Store unique IDs to prevent repeated installation of app
bool IsPMUAppInstalled(std::string PMUID);
bool IsPDCAppInstalled(std::string PDCID);
bool IsWACAppInstalled(std::string WACID);

std::vector<std::string> uniquePMUs;
std::vector<std::string> uniquePDCs;
std::vector<std::string> uniqueWACs;

std::pair<int,int> pmu_nodes;
std::pair<int,int> pdc_nodes;
std::pair<int,int> wac_nodes;

std::vector<std::pair<int,int>> all_flows;

// Vectors to store the various node types
vector<int> com_nodes, agg_nodes, phy_nodes;

// Vectors to store source and destination edges to fail
vector<int> srcedge, dstedge;

//Trace file
std::ofstream tracefile;

std::ofstream flowfile;


int
main(int argc, char* argv[])
{

  std::string PayloadSizeClassI = "1000";
  std::string PayloadSizeClassII="1000";
  std::string PayloadSizeClassIII="1000";
  std::string configFilePath="";
  float Class_1_Rate = 100.0;
  float Class_2_Rate = 120.0;
  float Class_3_Rate = 200.0;
  std::string TB_Size_class_1 = "0";
  std::string TB_Size_class_2 = "0";
  std::string TB_Size_class_3 = "0";
  std::string Token_gene_rate_Class_1 = "0";
  std::string Token_gene_rate_Class_2 = "0";
  std::string Token_gene_rate_Class_3 = "0";
  int Rate_Range = 0.0;
  int run=0;		
  int Test=0;

  // Read optional command-line parameters (e.g., enable visualizer with ./waf --run=<> --visualize
  CommandLine cmd;

  cmd.AddValue("Run", "Run", run);
  cmd.AddValue("Test", "Test", Test);
  cmd.AddValue("configFilePath", "configFilePath", configFilePath);
  cmd.AddValue("TB_Size_class_1", "TB_Size_class_1", TB_Size_class_1);
  cmd.AddValue("TB_Size_class_2", "TB_Size_class_2", TB_Size_class_2);
  cmd.AddValue("TB_Size_class_3", "TB_Size_class_3", TB_Size_class_3);
  cmd.AddValue("Token_gene_rate_Class_1", "Token_gene_rate_Class_1", Token_gene_rate_Class_1);
  cmd.AddValue("Token_gene_rate_Class_2", "Token_gene_rate_Class_2", Token_gene_rate_Class_2);
  cmd.AddValue("Token_gene_rate_Class_3", "Token_gene_rate_Class_3", Token_gene_rate_Class_3);
  cmd.AddValue("Rate_Range", "Rate_Range", Rate_Range);

  cmd.Parse(argc, argv);

  Token_gene_rate_Class_3 = std::to_string(std::stoi(Token_gene_rate_Class_3)+600);
   
  std::cout << "Test " << Test << std::endl;
  std::cout << "configFilePath " << configFilePath << std::endl;
  std::cout << "Class_1_Rate " << Class_1_Rate << std::endl;
  std::cout << "Class_2_Rate " << Class_2_Rate << std::endl;
  std::cout << "Class_3_Rate " << Class_3_Rate << std::endl;
  std::cout << "TB_Size_class_1 " << TB_Size_class_1 << std::endl;
  std::cout << "TB_Size_class_2 " << TB_Size_class_2 << std::endl;
  std::cout << "TB_Size_class_3 " << TB_Size_class_3 << std::endl;
  std::cout << "Token_gene_rate_Class_1 " << Token_gene_rate_Class_1 << std::endl;
  std::cout << "Token_gene_rate_Class_2 " << Token_gene_rate_Class_2 << std::endl;
  std::cout << "Token_gene_rate_Class_3 " << Token_gene_rate_Class_3 << std::endl;
 
 
  // Open the configuration file for reading
  ifstream configFile (configFilePath, std::ios::in);
  std::string strLine;
  bool gettingNodeCount = false, buildingNetworkTopo = false, attachingWACs = false, attachingPMUs = false, attachingPDCs = false, flowPMUtoPDC = false, flowPMUtoWAC = false;
  bool failLinks = false, injectData = false;
  std::vector<std::string> netParams;

  NodeContainer nodes;
  int nodeCount = 0;
  std::pair<int,int> flow_pair;
  int bgdNodeCount = 1;
  std::string bgdNamePrefix;
  unordered_map<int,int> used;
  unordered_map<int,int> usedS;
  unordered_map<int,int> usedR;


  // Setting default parameters for PointToPoint links and channels
  // Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue("10"));
  
  Config::SetDefault("ns3::QueueBase::MaxSize", StringValue("20p"));
  PointToPointHelper p2p;

  ndn::GlobalRoutingHelper ndnGlobalRoutingHelper;

  ndn::AppHelper producerHelper("ns3::ndn::QoSProducer");
  ndn::AppHelper consumerHelper("ns3::ndn::QoSConsumer");

  flowfile.open("ndn_all_flows.csv", std::ios::out);
  srand(run);
  if (configFile.is_open ()) {
        while (std::getline(configFile, strLine)) {

                //Determine what operation is ongoing while reading the config file
		if(strLine.substr(0,7) == "BEG_000") { gettingNodeCount = true; continue; }
		if(strLine.substr(0,7) == "END_000") {
			//Create nodes
			nodeCount++; //Counter increment needed because nodes start from 1 not 0
			gettingNodeCount = false;
			nodes.Create(nodeCount);
			
			continue; 
		}
		if(strLine.substr(0,7) == "BEG_001") { buildingNetworkTopo = true; continue; }
		if(strLine.substr(0,7) == "END_001") { buildingNetworkTopo = false; continue; }
		if(strLine.substr(0,7) == "BEG_002") { attachingWACs = true; continue; }
		if(strLine.substr(0,7) == "END_002") { attachingWACs = false; continue; }
		if(strLine.substr(0,7) == "BEG_003") { attachingPMUs = true; continue; }
		if(strLine.substr(0,7) == "END_003") { attachingPMUs = false; continue; }
		if(strLine.substr(0,7) == "BEG_004") { attachingPDCs = true; continue; }
		if(strLine.substr(0,7) == "END_004") { 
			attachingPDCs = false; 
			ndn::StackHelper ndnHelper; 
			ndnHelper.InstallAll();

			ndnGlobalRoutingHelper.Install(nodes);

			continue; 
		}
		if(strLine.substr(0,7) == "BEG_005") { flowPMUtoPDC = true; continue; }
		if(strLine.substr(0,7) == "END_005") { flowPMUtoPDC = false; uniquePMUs.clear(); continue; }
		if(strLine.substr(0,7) == "BEG_006") { flowPMUtoWAC = true; continue; }
		if(strLine.substr(0,7) == "END_006") { flowPMUtoWAC = false; uniquePMUs.clear(); continue; }
		if(strLine.substr(0,7) == "BEG_100") { failLinks = true; continue; }
                if(strLine.substr(0,7) == "END_100") { failLinks = false; continue; }
		if(strLine.substr(0,7) == "BEG_101") { injectData = true; continue; }
                if(strLine.substr(0,7) == "END_101") { injectData = false; continue; }


		if(gettingNodeCount == true) {
                        //Getting number of nodes to create
                        netParams = SplitString(strLine);
			nodeCount = stoi(netParams[1]);

			//Store node ID ranges for various node types
			if(netParams[2] == "wacs") { wac_nodes.first = std::stoi(netParams[0]); wac_nodes.second = std::stoi(netParams[1]); } 
			if(netParams[2] == "pmus") { pmu_nodes.first = std::stoi(netParams[0]); pmu_nodes.second = std::stoi(netParams[1]); }
			if(netParams[2] == "pdcs") { pdc_nodes.first = std::stoi(netParams[0]); pdc_nodes.second = std::stoi(netParams[1]); }
		}
		else if(buildingNetworkTopo == true) {
                        //Building network topology
                        netParams = SplitString(strLine);
                        p2p.SetDeviceAttribute("DataRate", StringValue(netParams[2]+"Mbps"));
                        p2p.SetChannelAttribute("Delay", StringValue(netParams[3]+"ms"));
          		p2p.Install(nodes.Get(std::stoi(netParams[0])), nodes.Get(std::stoi(netParams[1])));
		}
		else if(attachingWACs == true) {
                        //Attaching WACs to routers
                        netParams = SplitString(strLine);
                        p2p.SetDeviceAttribute("DataRate", StringValue(netParams[2]+"Mbps"));
                        p2p.SetChannelAttribute("Delay", StringValue(netParams[3]+"ms"));
                        p2p.Install(nodes.Get(std::stoi(netParams[0])), nodes.Get(std::stoi(netParams[1])));
		}
		else if(attachingPMUs == true) {
                        //Attaching PMUs to routers
                        netParams = SplitString(strLine);
                        p2p.SetDeviceAttribute("DataRate", StringValue(netParams[2]+"Mbps"));
                        p2p.SetChannelAttribute("Delay", StringValue(netParams[3]+"ms"));
                        p2p.Install(nodes.Get(std::stoi(netParams[0])), nodes.Get(std::stoi(netParams[1])));
		}
		else if(attachingPDCs == true) {
                        //Attaching PDCs to routers
                        netParams = SplitString(strLine);
                        p2p.SetDeviceAttribute("DataRate", StringValue(netParams[2]+"Mbps"));
                        p2p.SetChannelAttribute("Delay", StringValue(netParams[3]+"ms"));
                        p2p.Install(nodes.Get(std::stoi(netParams[0])), nodes.Get(std::stoi(netParams[1])));
		}
		else if(flowPMUtoPDC == true) {
			//Install apps on PDCs and PMUs for data exchange
			netParams = SplitString(strLine);

			//Install app on unique PDC IDs
			if (IsPDCAppInstalled(netParams[0]) == false) {
				producerHelper.SetPrefix("/power/typeI/data");
        			producerHelper.SetAttribute("Frequency", StringValue("0"));
        			producerHelper.Install(nodes.Get(std::stoi(netParams[0])));
                                used[std::stoi(netParams[0])] = 1;
				usedR[std::stoi(netParams[0])] = 1;
				// Setup node to originate prefixes for dynamic routing
  				ndnGlobalRoutingHelper.AddOrigin("/power/typeI/data", nodes.Get(std::stoi(netParams[0])));
				ndnGlobalRoutingHelper.AddOrigin("/power/typeI/data"+netParams[0], nodes.Get(std::stoi(netParams[0])));

			}

			//Install flow app on PMUs to send data to PDCs
			if (IsPMUAppInstalled(netParams[1]) == false) {
				  char temp[10];
	                        
				//sprintf(temp, "%lf", 1.0/(float(rand()%10+85)));
				sprintf(temp, "%lf", 1.0/(float(rand()%(Rate_Range*2)+(Class_1_Rate-Rate_Range))));
        			consumerHelper.SetPrefix("/power/typeI/data/phy" + netParams[1]);
                		consumerHelper.SetAttribute("Frequency", StringValue(temp)); //0.016 or 0.02
                		consumerHelper.SetAttribute("Subscription", IntegerValue(0));
                		consumerHelper.SetAttribute("PayloadSize", StringValue("200"));
                		consumerHelper.SetAttribute("RetransmitPackets", IntegerValue(0));
                		consumerHelper.SetAttribute("Offset", IntegerValue(0));
                		consumerHelper.SetAttribute("LifeTime", StringValue("100ms"));
                		consumerHelper.Install(nodes.Get(std::stoi(netParams[1])));
				used[std::stoi(netParams[1])] = 1;
				usedS[std::stoi(netParams[1])] = 1;
			}

			//Save the flow
			flow_pair.first = stoi(netParams[0]);
			flow_pair.second = stoi(netParams[1]);
			all_flows.push_back(flow_pair);

			//Write flow to file
			flowfile << netParams[0] << " " << netParams[1] << " TypeI" << std::endl;
		}
		else if(flowPMUtoWAC == true) {

                        //Install apps on PDCs and PMUs for data exchange
                        netParams = SplitString(strLine);

                        //Install app on unique PDC IDs
                        if (IsWACAppInstalled(netParams[0]) == false) {
                                producerHelper.SetPrefix("/power/typeII/data");
                                producerHelper.SetAttribute("Frequency", StringValue("0"));
                                producerHelper.Install(nodes.Get(std::stoi(netParams[0])));
                                used[std::stoi(netParams[0])] = 1;
				usedR[std::stoi(netParams[0])] = 1;

                                // Setup node to originate prefixes for dynamic routing
                                ndnGlobalRoutingHelper.AddOrigin("/power/typeII/data", nodes.Get(std::stoi(netParams[0])));
                                ndnGlobalRoutingHelper.AddOrigin("/power/typeII/data"+netParams[0], nodes.Get(std::stoi(netParams[0])));
                        }

                        //Install flow app on PMUs to send data to WACs
                        if (IsPMUAppInstalled(netParams[1]) == false) {
                                char temp[10];
                                //sprintf(temp, "%lf", 1.0/(float(rand()%10+145)));
                                sprintf(temp, "%lf", 1.0/(float(rand()%(Rate_Range*2)+(Class_2_Rate-Rate_Range))));			
                           
                                consumerHelper.SetPrefix("/power/typeII/data/phy" + netParams[1]);
                                consumerHelper.SetAttribute("Frequency", StringValue(temp)); //0.016 or 0.02
                                consumerHelper.SetAttribute("Subscription", IntegerValue(0));
                                consumerHelper.SetAttribute("PayloadSize", StringValue("200"));
                                consumerHelper.SetAttribute("RetransmitPackets", IntegerValue(0));
                                consumerHelper.SetAttribute("Offset", IntegerValue(0));
                                consumerHelper.SetAttribute("LifeTime", StringValue("100ms"));
                                consumerHelper.Install(nodes.Get(std::stoi(netParams[1])));
                                used[std::stoi(netParams[1])] = 1;
				usedS[std::stoi(netParams[1])] = 1;
                        } 

			//Save the flow
                        flow_pair.first = stoi(netParams[0]);
                        flow_pair.second = stoi(netParams[1]);
                        all_flows.push_back(flow_pair);
			//Write flow to file
                        flowfile << netParams[0] << " " << netParams[1] << " TypeII" << std::endl;

                }
		else if(failLinks == true) {

                        //Schedule the links to fail
                        netParams = SplitString(strLine);

			Simulator::Schedule(Seconds( ((double)stod(netParams[2])) ), ndn::LinkControlHelper::FailLink, nodes.Get(stoi(netParams[0])), nodes.Get(stoi(netParams[1])));
                        Simulator::Schedule(Seconds( ((double)stod(netParams[3])) ), ndn::LinkControlHelper::UpLink, nodes.Get(stoi(netParams[0])), nodes.Get(stoi(netParams[1])));

		}
		else if(injectData == true) {

                        std::cout<<strLine<<std::endl;
			//Install app on target node for data injection
			bgdNamePrefix = "/power/typeIII/data" + std::to_string(bgdNodeCount);
			producerHelper.SetPrefix(bgdNamePrefix);
                        producerHelper.SetAttribute("Frequency", StringValue("0"));
                        producerHelper.Install(nodes.Get(std::stoi(netParams[0])));
			used[std::stoi(netParams[0])] = 1;
			usedR[std::stoi(netParams[0])] = 1;

                        // Setup node to originate prefixes for dynamic routing
                        ndnGlobalRoutingHelper.AddOrigin(bgdNamePrefix, nodes.Get(std::stoi(netParams[0])));
                        char temp[10];
                        //sprintf(temp, "%lf", 1.0/(float(rand()%10+295)));
                        sprintf(temp, "%lf", 1.0/(float(rand()%(Rate_Range*2)+(Class_3_Rate-Rate_Range))));

			consumerHelper.SetPrefix(bgdNamePrefix + "/phy" + netParams[1]);
                        consumerHelper.SetAttribute("Frequency", StringValue(temp)); //0.0002 = 5000pps
                        consumerHelper.SetAttribute("Subscription", IntegerValue(0));
                        consumerHelper.SetAttribute("PayloadSize", StringValue("1024"));
                        consumerHelper.SetAttribute("RetransmitPackets", IntegerValue(0));
                        consumerHelper.SetAttribute("Offset", IntegerValue(0));
                        consumerHelper.SetAttribute("LifeTime", StringValue("200ms"));
                        ApplicationContainer bgdApps = consumerHelper.Install(nodes.Get(std::stoi(netParams[1])));
			used[std::stoi(netParams[1])] = 1;
			usedS[std::stoi(netParams[1])] = 1;
                        //bgdApps.Start (Seconds (std::stod(netParams[2])));
                        //bgdApps.Stop (Seconds (std::stod(netParams[3])));
                        flow_pair.first = stoi(netParams[0]);
                        flow_pair.second = stoi(netParams[1]);
                        all_flows.push_back(flow_pair);
                        netParams = SplitString(strLine);

                       flowfile << netParams[0] << " " << netParams[1] << " TypeIII "<<"data" + std::to_string(bgdNodeCount)<< std::endl;

			bgdNodeCount += 1;

		}
		else {
			//std::cout << "reading something else " << strLine << std::endl;
		}	
 
	} //end while
  } //endif
  else {
	std::cout << "Cannot open configuration file!!!" << std::endl;
	exit(1);
  }

  configFile.close();

  std::cout << "All done setting up" << std::endl;
  //Define the routing strategies per prefix
 ndn::StrategyChoiceHelper::InstallAll("/power/typeI/data", "/localhost/nfd/strategy/multicast");
  ndn::StrategyChoiceHelper::InstallAll("/power/typeII/data", "/localhost/nfd/strategy/multicast");
  ndn::StrategyChoiceHelper::InstallAll("/power/typeIII", "/localhost/nfd/strategy/best-route");
 //ndn::StrategyChoiceHelper::InstallAll("/", "/localhost/nfd/strategy/qos");

  ndn::AppHelper tokenHelper("ns3::ndn::TokenBucketDriver");
  tokenHelper.SetAttribute( "FillRate1", StringValue( Token_gene_rate_Class_1 ) ); 
  tokenHelper.SetAttribute( "Capacity1", StringValue( TB_Size_class_1 ) ); 
  tokenHelper.SetAttribute( "FillRate2", StringValue( Token_gene_rate_Class_2 ) ); 
  tokenHelper.SetAttribute( "Capacity2", StringValue( TB_Size_class_2 ) ); 
  tokenHelper.SetAttribute( "FillRate3", StringValue( Token_gene_rate_Class_3 ) ); 
  tokenHelper.SetAttribute( "Capacity3", StringValue( TB_Size_class_3 ) ); 

  tokenHelper.Install(nodes.Get(0));
    for (int i=0; i<(int)nodes.size(); i++) {
	if(used[i] == 0){
          ndn::StrategyChoiceHelper::Install(nodes.Get(i),"power/typeI/data", "/localhost/nfd/strategy/qos");
	  ndn::StrategyChoiceHelper::Install(nodes.Get(i),"power/typeII/data", "/localhost/nfd/strategy/qos");
          ndn::StrategyChoiceHelper::Install(nodes.Get(i),"power/typeIII", "/localhost/nfd/strategy/qos");
          ndn::StrategyChoiceHelper::Install(nodes.Get(i),"/", "/localhost/nfd/strategy/qos");

	  //tokenHelper.Install(nodes.Get(i));
        }
    }

  // Populate routing table for nodes
  //ndn::GlobalRoutingHelper::CalculateRoutes();
  ndn::GlobalRoutingHelper::CalculateAllPossibleRoutes();

  //Open trace file for writing
   char trace[100];
   sprintf( trace, "ndn-case%d-run%d.csv",Test, run );

   tracefile.open(trace, std::ios::out);
   tracefile << "nodeid, event, name, payloadsize, time" << std::endl;

   std::ofstream overheadfile;

  std::string strcallback;

  for ( int i=0; i<( int )nodes.size(); i++ ) {
     if( usedS[i] == 1 ){
        strcallback = "/NodeList/" + std::to_string( i ) + "/ApplicationList/" + "*/SentInterest";
        Config::ConnectWithoutContext( strcallback, MakeCallback( &SentInterestCallbackPhy ) );
     }
  }
  for ( int i=0; i<( int )nodes.size(); i++ ) {
     if( usedR[i] == 1 ){
        strcallback = "/NodeList/" + std::to_string( i ) + "/ApplicationList/" + "*/ReceivedInterest";
	Config::ConnectWithoutContext( strcallback, MakeCallback( &ReceivedInterestCallbackCom ) );
     }
  }

  clock_t start, end;
  double cpu_time_used;
  start = clock();

  Simulator::Stop(Seconds(2.0));
  Simulator::Run();
  Simulator::Destroy();
  end = clock();
  cpu_time_used = ( ( double ) ( end - start ) ) / CLOCKS_PER_SEC;
  std::cout<<"The simulation took "<<cpu_time_used<<std::endl;

  return 0;
}

//Check if source/destination flow is permitted in config file (one-to-many, many-to-one flows)
bool FlowPermitted (int dstnode, int srcnode) {

	for (int i=0; i<(int)all_flows.size(); i++) {
		if((all_flows[i].first == dstnode) && (all_flows[i].second == srcnode)) {
			//Flow found
			return true;
		}
	}
	return false;
}

//Get source node ID from which payload interest came from
uint32_t GetSourceNodeID (std::string name) {

	//Find first occurence of "/" and get remaining string to its right
	std::size_t pos = name.find("/");
	std::string remstr = name.substr(pos+1,string::npos);

	//Find second occurence of "/" and get remaining string to its right
        pos = remstr.find("/");
        remstr = remstr.substr(pos+1,string::npos);

	//Find third occurence of "/" and get remaining string to its right
        pos = remstr.find("/");
        remstr = remstr.substr(pos+1,string::npos);

	//Find fourth occurence of "/" and get remaining string to its right
        pos = remstr.find("/");
        remstr = remstr.substr(pos+1,string::npos);

	//Find fifth occurence of "/" and get source node ID
        pos = remstr.find("/");
        remstr = remstr.substr(3,pos-3);

	return stoi(remstr);
}

 
//Split a string delimited by space
std::vector<std::string> SplitString(std::string strLine) {
        std::string str = strLine;
        std::vector<std::string> result;
        std::istringstream isstr(str);
        for(std::string str; isstr >> str; )
                result.push_back(str);

        return result;
}

//Store unique PMU IDs to prevent repeated installation of app
bool IsPMUAppInstalled(std::string PMUID) {
        for (int i=0; i<(int)uniquePMUs.size(); i++) {
                if(PMUID.compare(uniquePMUs[i]) == 0) {
                        return true;
                }
        }

        uniquePMUs.push_back(PMUID);
        return false;
}

//Store unique PDC IDs to prevent repeated installation of app
bool IsPDCAppInstalled(std::string PDCID) {
	for (int i=0; i<(int)uniquePDCs.size(); i++) {
		if(PDCID.compare(uniquePDCs[i]) == 0) {
			return true;
		}
	}

	uniquePDCs.push_back(PDCID);
	return false;
}

//Store unique WAC IDs to prevent repeated installation of app
bool IsWACAppInstalled(std::string WACID) {
        for (int i=0; i<(int)uniqueWACs.size(); i++) {
                if(WACID.compare(uniqueWACs[i]) == 0) {
                        return true;
                }
        }

        uniqueWACs.push_back(WACID);
        return false;
}


//Define callbacks for writing to tracefile
void SentInterestCallbackPhy(uint32_t nodeid, shared_ptr<const ndn::Interest> interest) {
        if (interest->getSubscription() == 1 || interest->getSubscription() == 2) {
                //Do not log subscription interests from phy nodes
        }
        else {
		tracefile << nodeid << ", sent, " << interest->getName() << ", " << interest->getPayloadLength() << ", " << std::fixed << setprecision(9)
			<< (Simulator::Now().GetNanoSeconds())/1000000000.0 << std::endl;
        }
}

void ReceivedInterestCallbackCom(uint32_t nodeid, shared_ptr<const ndn::Interest> interest) {
        if (interest->getSubscription() == 1 || interest->getSubscription() == 2) {
                //Do not log subscription interests received at com nodes
        }
        else {
		std::stringstream sstr;
		sstr << interest->getName();
		std::string iname;
		sstr >> iname;

		//Only log flows that are permitted in config file (eliminates redundant interests received from multiple interfaces and to other nodes)
		if (FlowPermitted((int)nodeid, (int)GetSourceNodeID(iname)) == true) {
			tracefile << nodeid << ", recv, " << interest->getName() << ", " << interest->getPayloadLength() << ", " << std::fixed << setprecision(9)
                         << (Simulator::Now().GetNanoSeconds())/1000000000.0 << std::endl;
		}
        }
}

void SentDataCallbackCom(uint32_t nodeid, shared_ptr<const ndn::Data> data) {
	if (
		(data->getName().toUri()).find("error") != std::string::npos ||
		(data->getName().toUri()).find("pmu") != std::string::npos ||
		(data->getName().toUri()).find("ami") != std::string::npos

	) {
		//Do not log data for error, PMU ACK, AMI ACK from compute
	}
	else {
		tracefile << nodeid << ", sent, " << data->getName() << ", 1024, " << std::fixed << setprecision(9) << (Simulator::Now().GetNanoSeconds())/1000000000.0 << std::endl;
	}
}

void ReceivedDataCallbackPhy(uint32_t nodeid, shared_ptr<const ndn::Data> data) {
	if ((data->getName().toUri()).find("error") != std::string::npos) {
		//Do not log data received in response to payload interest
	}
	else {
		tracefile << nodeid << ", recv, " << data->getName() << ", 1024, " << std::fixed << setprecision(9) << (Simulator::Now().GetNanoSeconds())/1000000000.0 << std::endl;
	}
}

void ReceivedInterestCallbackAgg(uint32_t nodeid, shared_ptr<const ndn::Interest> interest) {
        tracefile << nodeid << ", recv, " << interest->getName() << ", " << interest->getPayloadLength() << ", " << std::fixed << setprecision(9)
		<< (Simulator::Now().GetNanoSeconds())/1000000000.0 << std::endl;
}

void SentInterestCallbackAgg(uint32_t nodeid, shared_ptr<const ndn::Interest> interest) {
        tracefile << nodeid << ", sent, " << interest->getName() << ", " << interest->getPayloadLength() << ", " << std::fixed << setprecision(9)
		<< (Simulator::Now().GetNanoSeconds())/1000000000.0 << std::endl;
}

void GetComAggEdges() {

}

bool NodeInComm(int nodeid) {
	for (int i=0; i<(int)com_nodes.size(); i++) {
		if (com_nodes[i] == nodeid) {
			return true;
		}
	}
	return false;
}

bool NodeInAgg(int nodeid) {
        for (int i=0; i<(int)agg_nodes.size(); i++) {
                if (agg_nodes[i] == nodeid) {
                        return true;
                }
        }
        return false;
}


} // namespace ns3

int
main(int argc, char* argv[])
{
  return ns3::main(argc, argv);
}
