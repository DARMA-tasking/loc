/*
//@HEADER
// *****************************************************************************
//                             test_coordinator.cc
//                       DARMA/loc => Location Coordinator
//         Copyright 2019-2024 NTESS (see LICENSE for full details)
// *****************************************************************************
//@HEADER
*/

#include "parallel_harness.h"

#include <loc/coordinator.h>

namespace loc { namespace tests {

using CoordType = Coordinator<int, comm::CommMPI>;

// Fixed home rank for the test entities.
static constexpr NodeType kHome = 0;
static constexpr int kEntity = 42;

struct TestCoordinator : ParallelHarness {};

// Query an entity that lives on the home rank from a non-home rank; the
// location must resolve to the home rank, and a repeat query must hit the cache.
TEST_F(TestCoordinator, resolve_remote_and_cache) {
  if (comm.numRanks() < 2) {
    GTEST_SKIP() << "requires >= 2 ranks";
  }
  CoordType coord{comm};
  auto const me = coord.thisNode();

  if (me == kHome) {
    coord.registerEntity(kEntity, kHome);
  }

  NodeType resolved = no_node;
  bool done = false;
  if (me == 1) {
    coord.getLocation(kEntity, kHome, [&](NodeType n) {
      resolved = n;
      done = true;
    });
  }

  pump();

  if (me == 1) {
    EXPECT_TRUE(done);
    EXPECT_EQ(resolved, kHome);
    EXPECT_TRUE(coord.isCached(kEntity));

    // A second lookup is served synchronously from the cache.
    NodeType again = no_node;
    bool again_done = false;
    coord.getLocation(kEntity, kHome, [&](NodeType n) {
      again = n;
      again_done = true;
    });
    EXPECT_TRUE(again_done);
    EXPECT_EQ(again, kHome);
  }
}

// Existence of a never-registered entity is definitively false at the home.
TEST_F(TestCoordinator, exists_false_for_unknown) {
  if (comm.numRanks() < 2) {
    GTEST_SKIP() << "requires >= 2 ranks";
  }
  CoordType coord{comm};
  auto const me = coord.thisNode();

  bool got = false;
  bool exists = true;
  NodeType where = no_node;
  if (me == 1) {
    coord.entityExists(9999, kHome, [&](bool e, NodeType n) {
      exists = e;
      where = n;
      got = true;
    });
  }

  pump();

  if (me == 1) {
    EXPECT_TRUE(got);
    EXPECT_FALSE(exists);
    EXPECT_EQ(where, no_node);
  }
}

// After an entity migrates, the home directory reflects the new owner.
TEST_F(TestCoordinator, migration_updates_home) {
  if (comm.numRanks() < 2) {
    GTEST_SKIP() << "requires >= 2 ranks";
  }
  CoordType coord{comm};
  auto const me = coord.thisNode();

  // Entity starts on the home rank (0).
  if (me == kHome) {
    coord.registerEntity(kEntity, kHome);
  }
  pump();

  // Migrate 0 -> 1.
  if (me == kHome) {
    coord.entityEmigrated(kEntity, 1);
  }
  if (me == 1) {
    coord.entityImmigrated(kEntity, kHome, kHome);
  }
  pump();

  // The home rank, which no longer hosts the entity, resolves to the new owner.
  if (me == kHome) {
    NodeType resolved = no_node;
    bool done = false;
    coord.getLocation(kEntity, kHome, [&](NodeType n) {
      resolved = n;
      done = true;
    });
    pump();
    EXPECT_TRUE(done);
    EXPECT_EQ(resolved, 1);
  } else {
    pump();
  }
}

// A third rank that resolved an entity is refreshed eagerly when it migrates,
// without issuing a new request.
TEST_F(TestCoordinator, eager_refresh_after_migration) {
  if (comm.numRanks() < 3) {
    GTEST_SKIP() << "requires >= 3 ranks";
  }
  CoordType coord{comm};
  auto const me = coord.thisNode();

  if (me == kHome) {
    coord.registerEntity(kEntity, kHome);
  }

  if (me == 2) {
    coord.getLocation(kEntity, kHome, [](NodeType) {});
  }
  pump();

  // Entity migrates 0 -> 1; rank 2 held a cached resolution and is refreshed.
  if (me == kHome) {
    coord.entityEmigrated(kEntity, 1);
  }
  if (me == 1) {
    coord.entityImmigrated(kEntity, kHome, kHome);
  }
  pump();

  if (me == 2) {
    ASSERT_TRUE(coord.isCached(kEntity));
    NodeType resolved = no_node;
    bool done = false;
    coord.getLocation(kEntity, kHome, [&](NodeType n) {
      resolved = n;
      done = true;
    });
    // Served from the eagerly-refreshed cache, no round trip needed.
    EXPECT_TRUE(done);
    EXPECT_EQ(resolved, 1);
  } else {
    pump();
  }
}

}} /* end namespace loc::tests */
