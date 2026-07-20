/*
//@HEADER
// *****************************************************************************
//                                   lookup.h
//                       DARMA/loc => Location Coordinator
//         Copyright 2019-2024 NTESS (see LICENSE for full details)
// *****************************************************************************
//@HEADER
*/

#if !defined INCLUDED_LOC_LOOKUP_LOOKUP_H
#define INCLUDED_LOC_LOOKUP_LOOKUP_H

#include "loc/common.h"
#include "loc/cache/cache.h"
#include "loc/directory/directory.h"

namespace loc {

/**
 * \brief Combined lookup over the home-node directory and the local LRU cache.
 *
 * On the home node (\c this_node == home) locations are stored permanently in
 * the directory; elsewhere they are cached and may be evicted.
 */
template <typename KeyT, typename ValueT>
struct LocLookup {
  LocLookup(LocationSizeType const& in_max_cache_size, NodeType in_this_node)
    : max_cache_size_(in_max_cache_size),
      cache_(in_max_cache_size),
      this_node_(in_this_node)
  { }

  bool exists(KeyT const& key) const {
    return directory_.exists(key) or cache_.exists(key);
  }

  LocationSizeType getCacheSize() const { return cache_.getSize(); }

  ValueT const& get(KeyT const& key) {
    auto dir_iter = directory_.getIter(key);
    if (dir_iter != directory_.getIterEnd()) {
      return dir_iter->second;
    }
    return cache_.get(key);
  }

  void remove(KeyT const& key) {
    directory_.remove(key);
    cache_.remove(key);
  }

  void insert(KeyT const& key, NodeType const home, ValueT const& value) {
    // If this node is the home, maintain location in the permanent directory,
    // otherwise insert/update in the local cache of locations.
    if (this_node_ == home) {
      directory_.insert(key, value);
    } else {
      cache_.insert(key, value);
    }
  }

  void update(KeyT const& key, ValueT const& value) {
    auto dir_iter = directory_.getIter(key);
    if (dir_iter != directory_.getIterEnd()) {
      dir_iter->second = value;
    } else {
      cache_.insert(key, value);
    }
  }

  void clearCache() {
    cache_ = LocationCache<KeyT, ValueT>{max_cache_size_};
  }

  /// Get the whole directory.
  auto const& getDirectory() const { return directory_; }

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | max_cache_size_ | directory_ | cache_ | this_node_;
  }

private:
  LocationSizeType max_cache_size_ = 0;
  Directory<KeyT, ValueT> directory_;
  LocationCache<KeyT, ValueT> cache_;
  NodeType this_node_ = no_node;
};

} /* end namespace loc */

#endif /*INCLUDED_LOC_LOOKUP_LOOKUP_H*/
