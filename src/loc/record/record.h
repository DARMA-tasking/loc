/*
//@HEADER
// *****************************************************************************
//                                   record.h
//                       DARMA/loc => Location Coordinator
//         Copyright 2019-2024 NTESS (see LICENSE for full details)
// *****************************************************************************
//@HEADER
*/

#if !defined INCLUDED_LOC_RECORD_RECORD_H
#define INCLUDED_LOC_RECORD_RECORD_H

#include "loc/common.h"
#include "loc/record/state.h"

namespace loc {

/**
 * \brief A single location record: the last-known node for an entity along with
 * whether that node is this rank (local) or another rank (remote).
 *
 * Unlike the original VT record, \c updateNode takes the current rank
 * explicitly rather than reaching into a global context singleton, keeping the
 * record free of any communication-layer dependency.
 */
template <typename EntityID>
struct LocRecord {
  using LocStateType = eLocState;

  LocRecord() = default;

  LocRecord(
    EntityID const& in_id, LocStateType const& in_state, NodeType in_node
  ) : id_(in_id), state_(in_state), cur_node_(in_node)
  { }

  /// Update the node this record points at, classifying local vs. remote
  /// relative to \p this_node.
  void updateNode(NodeType new_node, NodeType this_node) {
    state_ = (new_node == this_node) ? eLocState::Local : eLocState::Remote;
    cur_node_ = new_node;
  }

  bool isLocal() const { return state_ == eLocState::Local; }
  bool isRemote() const { return state_ == eLocState::Remote; }
  NodeType getRemoteNode() const { return cur_node_; }
  EntityID getEntityID() const { return id_; }

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | id_ | state_ | cur_node_;
  }

private:
  EntityID id_{};
  LocStateType state_ = eLocState::Invalid;
  NodeType cur_node_ = no_node;
};

} /* end namespace loc */

#endif /*INCLUDED_LOC_RECORD_RECORD_H*/
