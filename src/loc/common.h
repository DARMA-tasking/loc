/*
//@HEADER
// *****************************************************************************
//                                  common.h
//                       DARMA/loc => Location Coordinator
//         Copyright 2019-2024 NTESS (see LICENSE for full details)
// *****************************************************************************
//@HEADER
*/

#if !defined INCLUDED_LOC_COMMON_H
#define INCLUDED_LOC_COMMON_H

#include <cassert>
#include <cstddef>
#include <cstdint>

namespace loc {

/// A rank in the communicator. Matches comm's getRank()/numRanks() (int).
using NodeType = int;

/// Size type for the location cache.
using LocationSizeType = std::size_t;

/// Sentinel for an unset node.
inline constexpr NodeType no_node = -1;

/// Default maximum number of cached location records per coordinator.
inline constexpr LocationSizeType default_max_cache_size = 4096;

/// Assertion used throughout loc's internal data structures.
#define locAssert(cond, msg) assert((cond) && (msg))

} /* end namespace loc */

#endif /*INCLUDED_LOC_COMMON_H*/
