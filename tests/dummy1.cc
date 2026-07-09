#include <gtest/gtest.h>

TEST(TestGtest, Dummy11) {
  EXPECT_STRNE("hello", "world");
}

TEST(TestGtest, Dummy12) {
  int gold = 42;
  EXPECT_EQ(7 * 6, gold);
}
