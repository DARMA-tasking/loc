/*
//@HEADER
// *****************************************************************************
//                            coordinator.impl.h
//                       DARMA/loc => Location Coordinator
//         Copyright 2019-2024 NTESS (see LICENSE for full details)
// *****************************************************************************
//@HEADER
*/

#if !defined INCLUDED_LOC_COORDINATOR_IMPL_H
#define INCLUDED_LOC_COORDINATOR_IMPL_H

#include "loc/coordinator.h"

namespace loc {

template <typename EntityID, typename Comm>
Coordinator<EntityID, Comm>::Coordinator(
  Comm& in_comm, LocationSizeType in_max_cache_size
) : comm_(in_comm),
    this_node_(in_comm.getRank()),
    recs_(in_max_cache_size, this_node_)
{
  handle_ = comm_.template registerInstanceCollective<ThisType>(this);
}

template <typename EntityID, typename Comm>
void Coordinator<EntityID, Comm>::registerEntity(
  EntityID const& id, NodeType home
) {
  local_registered_.insert(id);
  if (home == this_node_) {
    announceLocation(id, this_node_);
  } else {
    comm_.template send<&ThisType::updateHome>(home, handle_, id, this_node_);
  }
}

template <typename EntityID, typename Comm>
void Coordinator<EntityID, Comm>::entityImmigrated(
  EntityID const& id, NodeType home, NodeType /*from*/
) {
  registerEntity(id, home);
}

template <typename EntityID, typename Comm>
void Coordinator<EntityID, Comm>::entityEmigrated(
  EntityID const& id, NodeType new_node
) {
  local_registered_.erase(id);
  recs_.update(id, makeRec(id, new_node));
}

template <typename EntityID, typename Comm>
void Coordinator<EntityID, Comm>::unregisterEntity(EntityID const& id) {
  local_registered_.erase(id);
  recs_.remove(id);
}

template <typename EntityID, typename Comm>
void Coordinator<EntityID, Comm>::getLocation(
  EntityID const& id, NodeType home, NodeAction action
) {
  if (local_registered_.count(id) != 0) {
    action(this_node_);
    return;
  }
  if (recs_.exists(id)) {
    action(recs_.get(id).getRemoteNode());
    return;
  }
  auto& list = pending_[id];
  list.push_back(std::move(action));
  // Only the first waiter issues the request; the home is not this rank here
  // (otherwise it would be locally known once registered).
  if (list.size() == 1 && home != this_node_) {
    comm_.template send<&ThisType::locationRequest>(
      home, handle_, id, this_node_
    );
  }
}

template <typename EntityID, typename Comm>
void Coordinator<EntityID, Comm>::entityExists(
  EntityID const& id, NodeType home, ExistsAction action
) {
  if (local_registered_.count(id) != 0) {
    action(true, this_node_);
    return;
  }
  if (recs_.exists(id)) {
    action(true, recs_.get(id).getRemoteNode());
    return;
  }
  if (home == this_node_) {
    // The home is authoritative: unknown here means it does not exist.
    action(false, no_node);
    return;
  }
  auto& list = pending_exists_[id];
  list.push_back(std::move(action));
  if (list.size() == 1) {
    comm_.template send<&ThisType::existsRequest>(home, handle_, id, this_node_);
  }
}

template <typename EntityID, typename Comm>
bool Coordinator<EntityID, Comm>::isCached(EntityID const& id) const {
  return local_registered_.count(id) != 0 || recs_.exists(id);
}

template <typename EntityID, typename Comm>
void Coordinator<EntityID, Comm>::clearCache() {
  recs_.clearCache();
}

template <typename EntityID, typename Comm>
NodeType Coordinator<EntityID, Comm>::thisNode() const {
  return this_node_;
}

template <typename EntityID, typename Comm>
void Coordinator<EntityID, Comm>::updateHome(EntityID id, NodeType node) {
  announceLocation(id, node);
}

template <typename EntityID, typename Comm>
void Coordinator<EntityID, Comm>::locationRequest(
  EntityID id, NodeType requester
) {
  if (recs_.exists(id)) {
    loc_asks_[id].insert(requester);
    comm_.template send<&ThisType::resolveResponse>(
      requester, handle_, id, recs_.get(id).getRemoteNode()
    );
  } else if (local_registered_.count(id) != 0) {
    loc_asks_[id].insert(requester);
    comm_.template send<&ThisType::resolveResponse>(
      requester, handle_, id, this_node_
    );
  } else {
    // Not known yet: buffer until the entity registers / is updated here.
    pending_home_[id].push_back(requester);
  }
}

template <typename EntityID, typename Comm>
void Coordinator<EntityID, Comm>::resolveResponse(
  EntityID id, NodeType node
) {
  recs_.update(id, makeRec(id, node));
  flushPending(pending_, id, node);
}

template <typename EntityID, typename Comm>
void Coordinator<EntityID, Comm>::existsRequest(
  EntityID id, NodeType requester
) {
  if (recs_.exists(id)) {
    comm_.template send<&ThisType::existsResponse>(
      requester, handle_, id, true, recs_.get(id).getRemoteNode()
    );
  } else if (local_registered_.count(id) != 0) {
    comm_.template send<&ThisType::existsResponse>(
      requester, handle_, id, true, this_node_
    );
  } else {
    comm_.template send<&ThisType::existsResponse>(
      requester, handle_, id, false, no_node
    );
  }
}

template <typename EntityID, typename Comm>
void Coordinator<EntityID, Comm>::existsResponse(
  EntityID id, bool exists, NodeType node
) {
  if (exists) {
    recs_.update(id, makeRec(id, node));
  }
  auto it = pending_exists_.find(id);
  if (it != pending_exists_.end()) {
    for (auto& action : it->second) {
      action(exists, node);
    }
    pending_exists_.erase(it);
  }
}

template <typename EntityID, typename Comm>
auto Coordinator<EntityID, Comm>::makeRec(
  EntityID const& id, NodeType node
) const -> LocRecType {
  LocRecType rec{id, eLocState::Invalid, no_node};
  rec.updateNode(node, this_node_);
  return rec;
}

template <typename EntityID, typename Comm>
void Coordinator<EntityID, Comm>::flushPending(
  std::unordered_map<EntityID, std::vector<NodeAction>>& map,
  EntityID const& id, NodeType node
) {
  auto it = map.find(id);
  if (it != map.end()) {
    for (auto& action : it->second) {
      action(node);
    }
    map.erase(it);
  }
}

template <typename EntityID, typename Comm>
void Coordinator<EntityID, Comm>::announceLocation(
  EntityID const& id, NodeType node
) {
  recs_.insert(id, /*home=*/this_node_, makeRec(id, node));

  // Eagerly refresh caches of ranks we have already answered.
  auto ask_it = loc_asks_.find(id);
  if (ask_it != loc_asks_.end()) {
    for (auto asker : ask_it->second) {
      comm_.template send<&ThisType::resolveResponse>(asker, handle_, id, node);
    }
  }

  // Answer ranks that asked before the entity was known, and remember them
  // for future eager refreshes.
  auto pend_it = pending_home_.find(id);
  if (pend_it != pending_home_.end()) {
    auto& askers = loc_asks_[id];
    for (auto requester : pend_it->second) {
      comm_.template send<&ThisType::resolveResponse>(
        requester, handle_, id, node
      );
      askers.insert(requester);
    }
    pending_home_.erase(pend_it);
  }

  // Satisfy local waiters on this (home) rank.
  flushPending(pending_, id, node);
}

} /* end namespace loc */

#endif /*INCLUDED_LOC_COORDINATOR_IMPL_H*/
