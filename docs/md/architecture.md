# Architecture

## Layers

| Layer | Contents | Depends on |
|-------|----------|-----------|
| Internal data structures | `cache/` (LRU), `directory/` (home map), `lookup/` (cache+directory), `record/` (per-entity state) | nothing but the C++ standard library |
| Coordinator | `coordinator.h` — the resolver protocol | the internal structures + a `comm::Communicator` |

The internal structures are pure and single-rank testable. The coordinator adds
the distributed protocol.

## The resolver protocol

Every entity has a fixed **home** rank (a deterministic function of its id,
chosen by the embedder). The home holds the authoritative location in its
`directory`; other ranks hold cached resolutions that may be evicted.

Control messages are point-to-point `comm::send<&Coordinator::handler>` calls:

- `updateHome(id, node)` — a rank tells the home where an entity now lives.
- `locationRequest(id, requester)` — a rank asks the home for a location.
- `resolveResponse(id, node)` — the home answers a request (and eagerly refreshes
  previously-answered ranks when the location later changes).
- `existsRequest` / `existsResponse` — the existence query pair.

Requests that reach the home before the entity is known are buffered
(`pending_home_`) and answered on registration. Ranks that have been answered are
remembered (`loc_asks_`) and **eagerly refreshed** when the entity migrates, so
caches converge without re-querying.

## Instance registration is collective

The coordinator registers itself with the communicator
(`registerInstanceCollective`) in its constructor. Because the returned handle's
index must match across ranks, **every rank must construct its coordinator in the
same order**. This mirrors VT's objgroup collective-creation requirement and is
the embedder's responsibility.

## What is deliberately out of scope

- Message forwarding / routing of the entities' own messages (resolver-only).
- The multi-instance `LocationManager`: the embedder owns coordinator lifetime
  and instantiates one per logical entity space.
- Any objgroup / VT coupling: the sole communication dependency is the
  `comm::Communicator` concept, satisfied by `CommMPI` (pure MPI) or `CommVT`.
