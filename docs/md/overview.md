# loc — Location Coordinator

`loc` is a standalone, communication-backend-agnostic library that answers one
question for a set of migratable entities: **which rank currently hosts entity
X?** It tracks entity registration and migration, caches resolutions, and keeps
those caches coherent — all on top of the [`comm`](https://github.com/DARMA-tasking/comm)
communication abstraction, with no dependency on VT.

It is extracted from VT's location manager (`vt/src/vt/topos/location`) and
redesigned around `comm`'s `Communicator` concept.

## Resolver, not router

`loc` is a **resolver**: it tells you the node an entity lives on. It does *not*
forward the entity's own messages. Once `getLocation` hands back a node, the
embedder performs the actual send. This keeps `loc` free of message envelopes,
epochs, and eager/rendezvous routing — the pieces most entangled with a specific
runtime. (Message forwarding on top of `comm` is a possible later phase.)

## Public API

The one type you use is `loc::Coordinator<EntityID, Comm>`:

- `registerEntity(id, home)` / `entityImmigrated(id, home, from)` — an entity now
  lives on this rank.
- `entityEmigrated(id, new_node)` — an entity left this rank.
- `unregisterEntity(id)` — an entity no longer exists here.
- `getLocation(id, home, action)` — asynchronously resolve the hosting node;
  `action(node)` fires once known.
- `entityExists(id, home, action)` — asynchronously test existence;
  `action(exists, node)`.
- `isCached(id)`, `clearCache()`, `thisNode()`.

Asynchronous results are delivered while the embedder pumps `Comm::poll()`.

## Building

`loc` consumes an installed `comm` via `find_package(comm CONFIG)` (which brings
MPI, magistrate/checkpoint, and fmt transitively):

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/comm/install/cmake
cmake --build build --parallel
ctest --test-dir build --output-on-failure
mpirun -np 2 ./build/tests/loc_tests   # exercises the cross-rank protocol
```
