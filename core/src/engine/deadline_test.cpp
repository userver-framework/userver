#include <userver/engine/deadline.hpp>

#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

TEST(Deadline, Reachability) {
  const auto deadline = engine::Deadline::FromDuration(kMaxTestWaitTime);
  EXPECT_TRUE(deadline.IsReachable());
  EXPECT_FALSE(deadline.IsReached());

  constexpr auto is_reachable = engine::Deadline{}.IsReachable();
  EXPECT_FALSE(is_reachable);

  const auto left = deadline.TimeLeft();
  EXPECT_GE(left, std::chrono::nanoseconds{1});
  EXPECT_LE(left, kMaxTestWaitTime);
}

USERVER_NAMESPACE_END
