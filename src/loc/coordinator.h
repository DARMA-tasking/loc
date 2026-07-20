/*
//@HEADER
// *****************************************************************************
//                                coordinator.h
//                       DARMA/loc => Location Coordinator
//         Copyright 2019-2024 NTESS (see LICENSE for full details)
// *****************************************************************************
//@HEADER
*/

#if !defined INCLUDED_LOC_COORDINATOR_H
#define INCLUDED_LOC_COORDINATOR_H

#include "loc/common.h"
#include "loc/lookup/lookup.h"
#include "loc/record/record.h"

#include <comm/comm/comm_traits.h>

#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace loc {

/**
 * \brief Distributed location resolver for a set of migratable entities.
 *
 * A \c Coordinator answers the question "which rank currently hosts entity X"
 * and keeps that answer coherent as entities migrate. It is a *resolver* only:
 * it never forwards the entity's own messages. Once a location is resolved the
 * embedder performs the actual send.
 *
 * Each entity has a fixed *home* rank (a deterministic function of its id,
 * chosen by the embedder). The home holds the authoritative record; other ranks
 * cache resolutions and are refreshed eagerly when the location changes.
 *
 * All inter-rank traffic goes through the injected \c Comm (any type satisfying
 * comm::Communicator): the coordinator registers itself as a collective
 * instance and issues point-to-point control messages via
 * \c Comm::send<&method>. Because registration is collective, every rank must
 * construct its coordinator in the same order so instance handles line up.
 *
 * \tparam EntityID serializable, default-constructible, hashable entity id
 * \tparam Comm a communicator type satisfying comm::Communicator
 */
template <typename EntityID, typename Comm>
struct Coordinator {
  static_assert(
    comm::Communicator<Comm>,
    "Coordinator requires a Comm satisfying comm::Communicator"
  );

  using ThisType = Coordinator<EntityID, Comm>;
  using LocRecType = LocRecord<EntityID>;
  using HandleType = typename Comm::template HandleType<ThisType>;
  using NodeAction = std::function<void(NodeType)>;
  using ExistsAction = std::function<void(bool, NodeType)>;

  /**
   * \brief Construct and collectively register with the communicator.
   *
   * \param[in] in_comm the communicator (must outlive this coordinator)
   * \param[in] in_max_cache_size max cached (non-home) location records
   */
  explicit Coordinator(
    Comm& in_comm, LocationSizeType in_max_cache_size = default_max_cache_size
  ) : comm_(in_comm),
      this_node_(in_comm.getRank()),
      recs_(in_max_cache_size, this_node_)
  {
    handle_ = comm_.template registerInstanceCollective<ThisType>(this);
  }

  Coordinator(Coordinator const&) = delete;
  Coordinator(Coordinator&&) = delete;
  Coordinator& operator=(Coordinator const&) = delete;

  /**
   * \brief Register an entity as living on this rank.
   *
   * \param[in] id the entity id
   * \param[in] home the home rank for \c id
   */
  void registerEntity(EntityID const& id, NodeType home) {
    local_registered_.insert(id);
    if (home == this_node_) {
      announceLocation(id, this_node_);
    } else {
      comm_.template send<&ThisType::updateHome>(home, handle_, id, this_node_);
    }
  }

  /**
   * \brief Register an entity that immigrated here from another rank.
   *
   * Equivalent to \c registerEntity for resolution purposes: the entity now
   * lives here and the home is informed.
   */
  void entityImmigrated(EntityID const& id, NodeType home, NodeType /*from*/) {
    registerEntity(id, home);
  }

  /**
   * \brief Note that an entity has emigrated off this rank to \c new_node.
   *
   * The local record is repointed so local queries forward to the new node; the
   * home is refreshed by the destination's \c entityImmigrated.
   */
  void entityEmigrated(EntityID const& id, NodeType new_node) {
    local_registered_.erase(id);
    recs_.update(id, makeRec(id, new_node));
  }

  /**
   * \brief Unregister an entity that no longer lives here.
   */
  void unregisterEntity(EntityID const& id) {
    local_registered_.erase(id);
    recs_.remove(id);
  }

  /**
   * \brief Resolve the current location of an entity.
   *
   * \c action is invoked (possibly after communication with the home rank) with
   * the resolved node. Progress requires the embedder to pump \c Comm::poll.
   *
   * \param[in] id the entity id
   * \param[in] home the home rank for \c id
   * \param[in] action callback invoked with the resolved node
   */
  void getLocation(EntityID const& id, NodeType home, NodeAction action) {
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
      comm_.template send<&ThisType::locationRequest>(home, handle_, id, this_node_);
    }
  }

  /**
   * \brief Check whether an entity exists anywhere in the system.
   *
   * \param[in] id the entity id
   * \param[in] home the home rank for \c id
   * \param[in] action callback invoked with (exists, node)
   */
  void entityExists(EntityID const& id, NodeType home, ExistsAction action) {
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

  /// Whether a resolved location for \c id is held locally (local or cached).
  bool isCached(EntityID const& id) const {
    return local_registered_.count(id) != 0 || recs_.exists(id);
  }

  /// Drop all cached (non-home) resolutions.
  void clearCache() { recs_.clearCache(); }

  /// This rank.
  NodeType thisNode() const { return this_node_; }

public:
  //
  // Control-plane RPC handlers. These are invoked by the communicator on
  // message receipt (via Comm::send<&handler>); they are not part of the
  // user-facing API but must be public so their member-pointers are usable.
  //

  /// [home] Learn/refresh where an entity lives.
  void updateHome(EntityID id, NodeType node) {
    announceLocation(id, node);
  }

  /// [home] A rank asks where an entity lives.
  void locationRequest(EntityID id, NodeType requester) {
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

  /// [asker] Home answered a location request.
  void resolveResponse(EntityID id, NodeType node) {
    recs_.update(id, makeRec(id, node));
    flushPending(pending_, id, node);
  }

  /// [home] A rank asks whether an entity exists.
  void existsRequest(EntityID id, NodeType requester) {
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

  /// [asker] Home answered an existence request.
  void existsResponse(EntityID id, bool exists, NodeType node) {
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

private:
  /// Build a record for \c id pointing at \c node, classified vs. this rank.
  LocRecType makeRec(EntityID const& id, NodeType node) const {
    LocRecType rec{id, eLocState::Invalid, no_node};
    rec.updateNode(node, this_node_);
    return rec;
  }

  /// Invoke and clear all pending node-actions buffered for \c id.
  static void flushPending(
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

  /**
   * \brief [home] The authoritative location of \c id is now \c node.
   *
   * Records it in the home directory, refreshes previously-answered askers
   * eagerly, and satisfies any buffered requests and local waiters.
   */
  void announceLocation(EntityID const& id, NodeType node) {
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

private:
  Comm& comm_;
  NodeType this_node_ = no_node;
  HandleType handle_{};

  /// Entities physically registered on this rank.
  std::unordered_set<EntityID> local_registered_;

  /// Home directory + local cache of resolved locations.
  LocLookup<EntityID, LocRecType> recs_;

  /// [asker] Location callbacks awaiting a response, keyed by entity.
  std::unordered_map<EntityID, std::vector<NodeAction>> pending_;

  /// [asker] Existence callbacks awaiting a response, keyed by entity.
  std::unordered_map<EntityID, std::vector<ExistsAction>> pending_exists_;

  /// [home] Requesters that asked before the entity was known.
  std::unordered_map<EntityID, std::vector<NodeType>> pending_home_;

  /// [home] Ranks that have cached a resolution and want eager refreshes.
  std::unordered_map<EntityID, std::unordered_set<NodeType>> loc_asks_;
};

} /* end namespace loc */

#endif /*INCLUDED_LOC_COORDINATOR_H*/
