/*
//@HEADER
// *****************************************************************************
//                                   cache.h
//                       DARMA/loc => Location Coordinator
//         Copyright 2019-2024 NTESS (see LICENSE for full details)
// *****************************************************************************
//@HEADER
*/

#if !defined INCLUDED_LOC_CACHE_CACHE_H
#define INCLUDED_LOC_CACHE_CACHE_H

#include "loc/common.h"

#include <list>
#include <tuple>
#include <unordered_map>

namespace loc {

/**
 * \brief LRU cache of resolved locations, keyed by entity id.
 *
 * On overflow past \c max_size the least-recently-used entry is evicted. A
 * \c get or re-\c insert promotes an entry to most-recently-used.
 */
template <typename KeyT, typename ValueT>
struct LocationCache {
  using LookupType = std::tuple<KeyT, ValueT>;
  using CacheOrderedType = std::list<LookupType>;
  using ValueIter = typename CacheOrderedType::iterator;
  using LookupContainerType = std::unordered_map<KeyT, ValueIter>;

  explicit LocationCache(LocationSizeType const& in_max_size)
    : max_size_(in_max_size)
  { }

  LocationCache(LocationCache const&) = delete;
  LocationCache(LocationCache&&) = default;
  LocationCache& operator=(LocationCache const&) = default;

  bool exists(KeyT const& key) const {
    return lookup_.find(key) != lookup_.end();
  }

  LocationSizeType getSize() const { return max_size_; }

  ValueT const& get(KeyT const& key) {
    auto iter = lookup_.find(key);
    locAssert(iter != lookup_.end(), "Key must exist in cache");
    cache_.splice(cache_.begin(), cache_, iter->second);
    return std::get<1>(*iter->second);
  }

  void remove(KeyT const& key) {
    auto iter = lookup_.find(key);
    if (iter != lookup_.end()) {
      cache_.erase(iter->second);
      lookup_.erase(iter);
    }
  }

  void insert(KeyT const& key, ValueT const& value) {
    auto iter = lookup_.find(key);
    if (iter == lookup_.end()) {
      if (lookup_.size() + 1 > max_size_) {
        auto last_iter = cache_.crbegin();
        lookup_.erase(std::get<0>(*last_iter));
        cache_.pop_back();
      }
      cache_.push_front(std::make_tuple(key, value));
      lookup_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(key),
        std::forward_as_tuple(cache_.begin())
      );
    } else {
      std::get<1>(*iter->second) = value;
      cache_.splice(cache_.cbegin(), cache_, iter->second);
    }
  }

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | cache_ | max_size_ | lookup_;
  }

private:
  /// container for quick lookup
  LookupContainerType lookup_;
  /// the location records sorted in LRU order (front = most recent)
  CacheOrderedType cache_;
  /// the maximum size the cache is allowed to grow
  LocationSizeType max_size_;
};

} /* end namespace loc */

#endif /*INCLUDED_LOC_CACHE_CACHE_H*/
