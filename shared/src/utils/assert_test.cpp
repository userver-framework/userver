#include <utils/assert.hpp>

#include <gtest/gtest.h>

TEST(AssertTest, Ok) {
  EXPECT_NO_THROW(UASSERT(true));
  EXPECT_NO_THROW(UASSERT_MSG(true, "ok"));
  EXPECT_NO_THROW(YTX_INVARIANT(true, "ok"));
}
