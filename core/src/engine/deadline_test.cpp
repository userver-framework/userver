#include <userver/engine/deadline.hpp>

#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

TEST(Deadline, Reachability) {
  const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);
  EXPECT_TRUE(deadline.IsReachable());
  EXPECT_FALSE(deadline.IsReached());

  constexpr auto is_reachable = engine::Deadline{}.IsReachable();
  EXPECT_FALSE(is_reachable);

  const auto left = deadline.TimeLeft();
  EXPECT_GE(left, std::chrono::nanoseconds{1});
  EXPECT_LE(left, utest::kMaxTestWaitTime);
}

TEST(Deadline, Passed) {
  const auto deadline =
      engine::Deadline::FromDuration(std::chrono::seconds{-1});

  EXPECT_TRUE(deadline.IsReached());
}

TEST(Deadline, Overflow) {
  // This duration will overflow on converting to steady_clock
  // duration
  const auto very_large_duration = std::chrono::hours::max();
  EXPECT_FALSE(
      engine::Deadline::FromDuration(very_large_duration).IsReachable());
}

TEST(DeadlineDeathTest, Overflow) {
  // This duration will not overflow when converting between durations, but
  // will overflow timepoint
  // UASSERT will cause test to die.
  const auto duration_to_overflow_time_point =
      engine::Deadline::Duration::max();
  if constexpr (utils::impl::kEnableAssert) {
    ASSERT_DEATH(
        { engine::Deadline::FromDuration(duration_to_overflow_time_point); },
        "");
  } else {
    EXPECT_TRUE(engine::Deadline::FromDuration(duration_to_overflow_time_point)
                    .IsReachable());
  }
}

USERVER_NAMESPACE_END
