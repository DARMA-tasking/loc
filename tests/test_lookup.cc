/*
//@HEADER
// *****************************************************************************
//                               test_lookup.cc
//                       DARMA/loc => Location Coordinator
//         Copyright 2019-2024 NTESS (see LICENSE for full details)
// *****************************************************************************
//@HEADER
*/

#include <gtest/gtest.h>

#include <loc/lookup/lookup.h>

namespace loc { namespace tests {

using LookupType = LocLookup<int, NodeType>;

TEST(TestLocLookup, home_node_uses_permanent_directory) {
  constexpr NodeType this_node = 0;
  LookupType lookup{/*max_cache_size=*/2, this_node};

  // this_node == home: entries land in the directory and never evict
  lookup.insert(1, /*home=*/this_node, 10);
  lookup.insert(2, /*home=*/this_node, 20);
  lookup.insert(3, /*home=*/this_node, 30);

  EXPECT_TRUE(lookup.exists(1));
  EXPECT_TRUE(lookup.exists(2));
  EXPECT_TRUE(lookup.exists(3));
  EXPECT_EQ(lookup.get(1), 10);
  EXPECT_EQ(lookup.get(3), 30);
}

TEST(TestLocLookup, non_home_uses_evictable_cache) {
  constexpr NodeType this_node = 0;
  constexpr NodeType home = 1;
  LookupType lookup{/*max_cache_size=*/2, this_node};

  lookup.insert(1, home, 10);
  lookup.insert(2, home, 20);
  lookup.insert(3, home, 30); // evicts LRU (key 1)

  EXPECT_FALSE(lookup.exists(1));
  EXPECT_TRUE(lookup.exists(2));
  EXPECT_TRUE(lookup.exists(3));
}

TEST(TestLocLookup, remove_and_clear_cache) {
  constexpr NodeType this_node = 0;
  constexpr NodeType home = 1;
  LookupType lookup{/*max_cache_size=*/4, this_node};

  lookup.insert(1, home, 10);
  ASSERT_TRUE(lookup.exists(1));
  lookup.remove(1);
  EXPECT_FALSE(lookup.exists(1));

  lookup.insert(2, home, 20);
  ASSERT_TRUE(lookup.exists(2));
  lookup.clearCache();
  EXPECT_FALSE(lookup.exists(2));
}

TEST(TestLocLookup, update_existing_directory_entry) {
  constexpr NodeType this_node = 0;
  LookupType lookup{/*max_cache_size=*/4, this_node};

  lookup.insert(1, /*home=*/this_node, 10);
  lookup.update(1, 42);
  EXPECT_EQ(lookup.get(1), 42);
}

}} /* end namespace loc::tests */
