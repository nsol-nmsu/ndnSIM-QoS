/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * We implement a Quality of Service (QoS) aware network architecture that aims to 
 * satisfy low latency, high bandwidth, and high reliability requirements of 
 * communications by extending the features of NDN. In this architecture, we categorize 
 * traffic into three priority classes (high, medium, and low priority) to enable 
 * preferential treatment of traffic flows. The three classes were identified for a 
 * smart grid application, but can be generalized to increase or decrease the number of 
 * classes. For the smart grid application, the three classes correspond to: (i) protection 
 * traffic, which requires low latency and high reliability; (ii) control traffic, 
 * which needs high reliability; and (iii) best-effort traffic, which has no requirement constraints.
 *
 * This code extends the ndnSIM codebase which was developed by University of California, 
 * Los Angeles (see AUTHORS for list of new code contributors). 
 * 
 * Copyright (c) 2011-2020 Regents of the University of California and New Mexico State 
 * University. This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors 
 * and contributors.
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
 *
 **/

/**

 * \mainpage ndnQoS documentation (NMSU extension of ndnSIM v2.7)
 */

 * \mainpage ndnSIM QoS documentation
 *
 * Please refer to <a href="../index.html">ndnSIM QoS documentation page</a>
 */

// explicit instantiation and registering

/**
 * @brief ContentStore with LRU cache replacement policy
 */
template class ContentStoreImpl<lru_policy_traits>;
/**
 * @brief ContentStore with random cache replacement policy
 */

template class ContentStoreImpl<random_policy_traits>;

/**
 * @brief ContentStore with FIFO cache replacement policy
 */
template class ContentStoreImpl<fifo_policy_traits>;
