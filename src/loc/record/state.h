/*
//@HEADER
// *****************************************************************************
//                                   state.h
//                       DARMA/loc => Location Coordinator
//         Copyright 2019-2024 NTESS (see LICENSE for full details)
// *****************************************************************************
//@HEADER
*/

#if !defined INCLUDED_LOC_RECORD_STATE_H
#define INCLUDED_LOC_RECORD_STATE_H

#include <cstdint>

namespace loc {

/// Whether a location record refers to a local or remote entity.
enum class eLocState : int32_t {
  Local = 1,
  Remote = 2,
  Invalid = -1
};

} /* end namespace loc */

#endif /*INCLUDED_LOC_RECORD_STATE_H*/
