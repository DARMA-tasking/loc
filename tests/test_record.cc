/*
//@HEADER
// *****************************************************************************
//                               test_record.cc
//                       DARMA/loc => Location Coordinator
//         Copyright 2019-2024 NTESS (see LICENSE for full details)
// *****************************************************************************
//@HEADER
*/

#include <gtest/gtest.h>

#include <loc/record/record.h>
#include <loc/record/state.h>

namespace loc { namespace tests {

TEST(TestLocRecord, construct_and_accessors) {
  LocRecord<int> rec{7, eLocState::Local, 3};
  EXPECT_EQ(rec.getEntityID(), 7);
  EXPECT_EQ(rec.getRemoteNode(), 3);
  EXPECT_TRUE(rec.isLocal());
  EXPECT_FALSE(rec.isRemote());
}

TEST(TestLocRecord, update_node_classifies_local_vs_remote) {
  constexpr NodeType this_node = 2;
  LocRecord<int> rec{7, eLocState::Invalid, no_node};

  rec.updateNode(2, this_node);
  EXPECT_TRUE(rec.isLocal());
  EXPECT_FALSE(rec.isRemote());
  EXPECT_EQ(rec.getRemoteNode(), 2);

  rec.updateNode(5, this_node);
  EXPECT_FALSE(rec.isLocal());
  EXPECT_TRUE(rec.isRemote());
  EXPECT_EQ(rec.getRemoteNode(), 5);
}

}} /* end namespace loc::tests */
