#include <gtest/gtest.h>
#include <loc/dummy.h>

TEST(TestSource, Dummy2) {
  loc::dummy::Dummy dummy;
  int gold = 5;
  EXPECT_EQ(dummy.sum(2, 3), gold);
}
