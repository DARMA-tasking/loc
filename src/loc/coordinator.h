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
#include "loc/communicator.h"
#include "loc/lookup/lookup.h"
#include "loc/record/record.h"

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
 * loc::Communicator): the coordinator registers itself as a collective instance
 * and issues point-to-point control messages via
 * \c Comm::send<&method>. Because registration is collective, every rank must
 * construct its coordinator in the same order so instance handles line up.
 *
 * \tparam EntityID serializable, default-constructible, hashable entity id
 * \tparam Comm a communicator type satisfying loc::Communicator
 */
template <typename EntityID, typename Comm>
struct Coordinator {
  static_assert(
    Communicator<Comm, Coordinator<EntityID, Comm>>,
    "Coordinator requires a Comm satisfying loc::Communicator"
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
  );

  Coordinator(Coordinator const&) = delete;
  Coordinator(Coordinator&&) = delete;
  Coordinator& operator=(Coordinator const&) = delete;

  /**
   * \brief Register an entity as living on this rank.
   *
   * \param[in] id the entity id
   * \param[in] home the home rank for \c id
   */
  void registerEntity(EntityID const& id, NodeType home);

  /**
   * \brief Register an entity that immigrated here from another rank.
   *
   * Equivalent to \c registerEntity for resolution purposes: the entity now
   * lives here and the home is informed.
   */
  void entityImmigrated(EntityID const& id, NodeType home, NodeType from);

  /**
   * \brief Note that an entity has emigrated off this rank to \c new_node.
   *
   * The local record is repointed so local queries forward to the new node; the
   * home is refreshed by the destination's \c entityImmigrated.
   */
  void entityEmigrated(EntityID const& id, NodeType new_node);

  /**
   * \brief Unregister an entity that no longer lives here.
   */
  void unregisterEntity(EntityID const& id);

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
  void getLocation(EntityID const& id, NodeType home, NodeAction action);

  /**
   * \brief Check whether an entity exists anywhere in the system.
   *
   * \param[in] id the entity id
   * \param[in] home the home rank for \c id
   * \param[in] action callback invoked with (exists, node)
   */
  void entityExists(EntityID const& id, NodeType home, ExistsAction action);

  /// Whether a resolved location for \c id is held locally (local or cached).
  bool isCached(EntityID const& id) const;

  /// Drop all cached (non-home) resolutions.
  void clearCache();

  /// This rank.
  NodeType thisNode() const;

public:
  //
  // Control-plane RPC handlers. These are invoked by the communicator on
  // message receipt (via Comm::send<&handler>); they are not part of the
  // user-facing API but must be public so their member-pointers are usable.
  //

  /// [home] Learn/refresh where an entity lives.
  void updateHome(EntityID id, NodeType node);

  /// [home] A rank asks where an entity lives.
  void locationRequest(EntityID id, NodeType requester);

  /// [asker] Home answered a location request.
  void resolveResponse(EntityID id, NodeType node);

  /// [home] A rank asks whether an entity exists.
  void existsRequest(EntityID id, NodeType requester);

  /// [asker] Home answered an existence request.
  void existsResponse(EntityID id, bool exists, NodeType node);

private:
  /// Build a record for \c id pointing at \c node, classified vs. this rank.
  LocRecType makeRec(EntityID const& id, NodeType node) const;

  /// Invoke and clear all pending node-actions buffered for \c id.
  static void flushPending(
    std::unordered_map<EntityID, std::vector<NodeAction>>& map,
    EntityID const& id, NodeType node
  );

  /**
   * \brief [home] The authoritative location of \c id is now \c node.
   *
   * Records it in the home directory, refreshes previously-answered askers
   * eagerly, and satisfies any buffered requests and local waiters.
   */
  void announceLocation(EntityID const& id, NodeType node);

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

#include "loc/coordinator.impl.h"

#endif /*INCLUDED_LOC_COORDINATOR_H*/
