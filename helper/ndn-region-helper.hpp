/*
 * ndn-region-helper.hpp
 *
 *  Created on: Jan 19, 2022
 *      Author: sharad
 */
#ifndef NS_3_SRC_NDNSIM_HELPER_NDN_REGION_HELPER_HPP_
#define NS_3_SRC_NDNSIM_HELPER_NDN_REGION_HELPER_HPP_
#include <string>
#include <vector>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdlib.h>

namespace ns3{
class regionHelper {
	public:
		std::string getRegion(std::string nodeNo);
		static std::string getAllRegions(std::string nodeNo);

		static std::vector<std::string>SplitRegion( std::string strLine);
};
}
#endif /* NS_3_SRC_NDNSIM_HELPER_NDN_REGION_HELPER_HPP_ */
