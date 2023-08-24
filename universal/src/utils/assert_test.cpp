#include <userver/utils/assert.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

TEST(AssertTest, Ok) {
  EXPECT_NO_THROW(UASSERT(true));
  EXPECT_NO_THROW(UASSERT_MSG(true, "ok"));
  EXPECT_NO_THROW(UINVARIANT(true, "ok"));
}

TEST(AssertTest, StringView) {
  std::string_view message = "Testing";
  EXPECT_NO_THROW(UASSERT_MSG(true, message));
  EXPECT_NO_THROW(UINVARIANT(true, message));
}

USERVER_NAMESPACE_END
