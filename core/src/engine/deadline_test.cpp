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

TEST(Deadline, DurationMax) {
    const auto deadline = engine::Deadline::FromDuration(engine::Deadline::Duration::max());
    EXPECT_FALSE(deadline.IsReachable());
}

TEST(Deadline, GreaterThanDurationMax) {
    const auto very_large_duration = std::chrono::hours::max();
    EXPECT_GT(very_large_duration, std::chrono::duration_cast<std::chrono::hours>(engine::Deadline::Duration::max()));
    EXPECT_FALSE(engine::Deadline::FromDuration(very_large_duration).IsReachable());
}

TEST(Deadline, Passed) {
    const auto deadline = engine::Deadline::FromDuration(std::chrono::seconds{-1});
    EXPECT_TRUE(deadline.IsReached());
}

TEST(DeadlineTest, Overflow) {
    const auto duration =
        engine::Deadline::TimePoint::max() - engine::Deadline::Clock::now() + engine::Deadline::Duration{1};
    EXPECT_LT(duration, engine::Deadline::Duration::max());
    EXPECT_FALSE(engine::Deadline::FromDuration(duration).IsReachable());
}

TEST(Deadline, TimePoint) {
    // special cases
    // Unreachable Deadline
    EXPECT_EQ(engine::Deadline{}.GetTimePoint(), engine::Deadline::TimePoint::max());
    // Passed Deadline
    EXPECT_EQ(engine::Deadline::Passed().GetTimePoint(), engine::Deadline::TimePoint::min());

    // common use
    const auto tp = engine::Deadline::Clock::now();
    const auto deadline = engine::Deadline::FromTimePoint(tp);
    EXPECT_EQ(deadline.GetTimePoint(), tp);
}

USERVER_NAMESPACE_END
