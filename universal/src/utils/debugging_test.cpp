#include <userver/utils/debugging.hpp>

#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

TEST(DebuggingTest, IsDebuggerPresent) { EXPECT_FALSE(utils::IsDebuggerPresent()); }

USERVER_NAMESPACE_END
