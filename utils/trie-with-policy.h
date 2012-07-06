/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011 University of California, Los Angeles
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 */

#ifndef TRIE_WITH_POLICY_H_
#define TRIE_WITH_POLICY_H_

#include "trie.h"

namespace ndnSIM
{

template<typename FullKey,
         typename PayloadTraits,
         typename PolicyTraits
         >
class trie_with_policy
{
public:
  typedef trie< FullKey,
                typename PayloadTraits::payload_type,
                PayloadTraits,
                typename PolicyTraits::policy_hook_type > parent_trie;

  typedef typename parent_trie::iterator iterator;

  typedef typename PolicyTraits::template policy<
    trie_with_policy<FullKey, PayloadTraits, PolicyTraits>,
    parent_trie,
    typename PolicyTraits::template container_hook<parent_trie>::type >::type policy_container;

  inline
  trie_with_policy (size_t bucketSize = 10, size_t bucketIncrement = 10)
    : trie_ ("", bucketSize, bucketIncrement)
    , policy_ (*this)
  {
  }

  inline std::pair< iterator, bool >
  insert (const FullKey &key, typename PayloadTraits::const_pointer_type payload)
  {
    std::pair<iterator, bool> item =
      trie_.insert (key, payload);

    if (item.second) // real insert
      {
        bool ok = policy_.insert (s_iterator_to (item.first));
        if (!ok)
          {
            item.first->erase (); // cannot insert
            return std::make_pair (end (), false);
          }
      }
    else
      {
        item.first->set_payload (payload);
        policy_.update (s_iterator_to (item.first));
      }

    return item;
  }

  inline void
  erase (iterator node)
  {
    if (node == end ()) return;

    policy_.erase (s_iterator_to (node));
    node->erase (); // will do cleanup here
  }

  /**
   * @brief Find a node that has the longest common prefix with key (FIB/PIT lookup)
   */
  inline iterator
  longest_prefix_match (const FullKey &key)
  {
    boost::tuple< iterator, bool, iterator > item = trie_.find (key);
    if (item.template get<0> () != trie_.end ())
      {
        policy_.lookup (s_iterator_to (item.template get<0> ()));
      }
    return item.template get<0> ();
  }

  /**
   * @brief Find a node that has prefix at least as the key (cache lookup)
   */
  inline iterator
  deepest_prefix_match (const FullKey &key)
  {
    iterator foundItem, lastItem;
    bool reachLast;
    boost::tie (foundItem, reachLast, lastItem) = trie_.find (key);

    // guard in case we don't have anything in the trie
    if (lastItem == trie_.end ())
      return trie_.end ();
    
    if (reachLast)
      {
        if (foundItem == trie_.end ())
          {
            foundItem = lastItem->find (); // should be something
          }
        policy_.lookup (s_iterator_to (foundItem));
        return foundItem;
      }
    else
      { // couldn't find a node that has prefix at least as key
        return trie_.end ();
      }
  }

  /**
   * @brief Find a node that has prefix at least as the key
   */
  template<class Predicate>
  inline iterator
  deepest_prefix_match (const FullKey &key, Predicate pred)
  {
    iterator foundItem, lastItem;
    bool reachLast;
    boost::tie (foundItem, reachLast, lastItem) = trie_.find (key);

    // guard in case we don't have anything in the trie
    if (lastItem == trie_.end ())
      return trie_.end ();
    
    if (reachLast)
      {
        foundItem = lastItem->find_if (pred); // may or may not find something
        if (foundItem == trie_.end ())
          {
            return trie_.end ();
          }
        policy_.lookup (s_iterator_to (foundItem));
        return foundItem;
      }
    else
      { // couldn't find a node that has prefix at least as key
        return trie_.end ();
      }
  }

  iterator end ()
  {
    return 0;
  }

  const parent_trie &
  getTrie () const { return trie_; }

  const policy_container &
  getPolicy () const { return policy_; }

  policy_container &
  getPolicy () { return policy_; }

  static inline iterator
  s_iterator_to (typename parent_trie::iterator item)
  {
    if (item == 0)
      return 0;
    else
      return &(*item);
  }
  
private:
  parent_trie      trie_;
  policy_container policy_;
};

} // ndnSIM

#endif // TRIE_WITH_POLICY_H_

