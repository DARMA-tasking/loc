/*
//@HEADER
// *****************************************************************************
//                                test_cache.cc
//                       DARMA/loc => Location Coordinator
//         Copyright 2019-2024 NTESS (see LICENSE for full details)
// *****************************************************************************
//@HEADER
*/

#include <gtest/gtest.h>

#include <loc/cache/cache.h>

namespace loc { namespace tests {

using CacheType = LocationCache<int, NodeType>;

TEST(TestLocationCache, insert_get_exists) {
  CacheType cache{4};
  EXPECT_EQ(cache.getSize(), 4u);
  EXPECT_FALSE(cache.exists(1));

  cache.insert(1, 10);
  cache.insert(2, 20);
  EXPECT_TRUE(cache.exists(1));
  EXPECT_TRUE(cache.exists(2));
  EXPECT_EQ(cache.get(1), 10);
  EXPECT_EQ(cache.get(2), 20);
}

TEST(TestLocationCache, reinsert_updates_value) {
  CacheType cache{4};
  cache.insert(1, 10);
  cache.insert(1, 99);
  EXPECT_TRUE(cache.exists(1));
  EXPECT_EQ(cache.get(1), 99);
}

TEST(TestLocationCache, remove) {
  CacheType cache{4};
  cache.insert(1, 10);
  ASSERT_TRUE(cache.exists(1));
  cache.remove(1);
  EXPECT_FALSE(cache.exists(1));
  // removing a missing key is a no-op
  cache.remove(42);
}

TEST(TestLocationCache, lru_eviction_of_least_recently_used) {
  CacheType cache{2};
  cache.insert(1, 10);
  cache.insert(2, 20);
  // touch key 1 so key 2 becomes least-recently-used
  EXPECT_EQ(cache.get(1), 10);
  // inserting a third entry evicts the LRU (key 2)
  cache.insert(3, 30);

  EXPECT_TRUE(cache.exists(1));
  EXPECT_FALSE(cache.exists(2));
  EXPECT_TRUE(cache.exists(3));
}

TEST(TestLocationCache, eviction_without_touch_drops_oldest) {
  CacheType cache{2};
  cache.insert(1, 10);
  cache.insert(2, 20);
  cache.insert(3, 30); // no touch: oldest (key 1) evicted

  EXPECT_FALSE(cache.exists(1));
  EXPECT_TRUE(cache.exists(2));
  EXPECT_TRUE(cache.exists(3));
}

}} /* end namespace loc::tests */
