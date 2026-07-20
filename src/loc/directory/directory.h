/*
//@HEADER
// *****************************************************************************
//                                 directory.h
//                       DARMA/loc => Location Coordinator
//         Copyright 2019-2024 NTESS (see LICENSE for full details)
// *****************************************************************************
//@HEADER
*/

#if !defined INCLUDED_LOC_DIRECTORY_DIRECTORY_H
#define INCLUDED_LOC_DIRECTORY_DIRECTORY_H

#include "loc/common.h"

#include <tuple>
#include <unordered_map>

namespace loc {

/**
 * \brief Permanent, home-node directory of entity locations.
 *
 * Unlike the cache, the directory never evicts: on the home node it is the
 * authoritative record of where an entity currently lives.
 */
template <typename KeyT, typename ValueT>
struct Directory {
  using DirectoryMapType = std::unordered_map<KeyT, ValueT>;

  Directory() = default;

  bool exists(KeyT const& key) const {
    return dir_.find(key) != dir_.end();
  }

  std::size_t getSize() const { return dir_.size(); }

  ValueT const& get(KeyT const& key) {
    auto iter = dir_.find(key);
    locAssert(iter != dir_.end(), "Key must exist in directory");
    return iter->second;
  }

  typename DirectoryMapType::iterator getIter(KeyT const& key) {
    return dir_.find(key);
  }

  typename DirectoryMapType::iterator getIterEnd() { return dir_.end(); }

  void remove(KeyT const& key) {
    auto iter = dir_.find(key);
    if (iter != dir_.end()) {
      dir_.erase(iter);
    }
  }

  void insert(KeyT const& key, ValueT const& value) {
    auto iter = dir_.find(key);
    if (iter == dir_.end()) {
      dir_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(key),
        std::forward_as_tuple(value)
      );
    } else {
      iter->second = value;
    }
  }

  /// Get the whole directory map.
  auto const& getMap() const { return dir_; }

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | dir_;
  }

private:
  DirectoryMapType dir_;
};

} /* end namespace loc */

#endif /*INCLUDED_LOC_DIRECTORY_DIRECTORY_H*/
