/*
//@HEADER
// *****************************************************************************
//                               communicator.h
//                       DARMA/loc => Location Coordinator
//         Copyright 2019-2024 NTESS (see LICENSE for full details)
// *****************************************************************************
//@HEADER
*/

#if !defined INCLUDED_LOC_COMMUNICATOR_H
#define INCLUDED_LOC_COMMUNICATOR_H

#include "loc/common.h"

#include <concepts>

namespace loc {

/**
 * \brief Minimal communication contract required by \c Coordinator.
 *
 * The concrete backend owns its instance-handle representation and message
 * transport. The coordinator additionally calls
 * \c Comm::send<&Coordinator::handler>(node, handle, args...), whose
 * handler-specific signatures are checked when those operations are
 * instantiated.
 */
template <typename Comm, typename Instance>
concept Communicator = requires(Comm& comm, Instance* instance) {
  typename Comm::template HandleType<Instance>;
  requires std::default_initializable<
    typename Comm::template HandleType<Instance>
  >;

  { comm.getRank() } -> std::convertible_to<NodeType>;
  {
    comm.template registerInstanceCollective<Instance>(instance)
  } -> std::convertible_to<typename Comm::template HandleType<Instance>>;
};

} /* end namespace loc */

#endif /*INCLUDED_LOC_COMMUNICATOR_H*/
