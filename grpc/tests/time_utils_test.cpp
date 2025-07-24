#include <gmock/gmock.h>
#include <userver/utest/utest.hpp>

#include <grpc/support/time.h>

#include <userver/engine/sleep.hpp>

#include <userver/ugrpc/time_utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr auto kBaseTimeout = std::chrono::milliseconds{200};

}  // namespace

UTEST(DeadlineToTimespec, Base) {
    const auto duration = std::chrono::milliseconds{500};
    const auto lo = engine::Deadline::FromDuration(duration);
    gpr_timespec t = ugrpc::DeadlineToTimespec(engine::Deadline::FromDuration(duration));
    const auto deadline = ugrpc::TimespecToDeadline(t);
    const auto hi = engine::Deadline::FromDuration(duration);
    EXPECT_TRUE(deadline.IsReachable());
    EXPECT_LT(lo, deadline);
    EXPECT_LT(deadline, hi);
}

UTEST(DeadlineToTimespec, FromUnreachableDeadline) {
    gpr_timespec t = ugrpc::DeadlineToTimespec(engine::Deadline{});
    EXPECT_EQ(t.tv_sec, gpr_inf_future(GPR_CLOCK_MONOTONIC).tv_sec);
    EXPECT_EQ(t.tv_nsec, 0);
    EXPECT_EQ(t.clock_type, GPR_CLOCK_MONOTONIC);
}

UTEST(DeadlineToTimespec, FromPassedDeadline) {
    gpr_timespec t = ugrpc::DeadlineToTimespec(engine::Deadline::Passed());
    EXPECT_EQ(t.tv_sec, gpr_inf_past(GPR_CLOCK_MONOTONIC).tv_sec);
    EXPECT_EQ(t.tv_nsec, 0);
    EXPECT_EQ(t.clock_type, GPR_CLOCK_MONOTONIC);
}

UTEST(DeadlineToTimespec, PassedAfterSleep) {
    const auto duration = std::chrono::milliseconds{500};
    gpr_timespec t = ugrpc::DeadlineToTimespec(engine::Deadline::FromDuration(duration));
    engine::InterruptibleSleepFor(duration);
    EXPECT_EQ(ugrpc::TimespecToDeadline(t), engine::Deadline::Passed());
}

UTEST(DurationToTimespec, FromDurationMax) {
    gpr_timespec t = ugrpc::DurationToTimespec(engine::Deadline::Duration::max());
    EXPECT_EQ(t.tv_sec, gpr_inf_future(GPR_TIMESPAN).tv_sec);
    EXPECT_EQ(t.tv_nsec, 0);
    EXPECT_EQ(t.clock_type, GPR_TIMESPAN);
}

UTEST(DurationToTimespec, FromMillisecondsMax) {
    gpr_timespec t = ugrpc::DurationToTimespec(std::chrono::milliseconds::max());
    EXPECT_EQ(t.tv_sec, gpr_inf_future(GPR_TIMESPAN).tv_sec);
    EXPECT_EQ(t.tv_nsec, 0);
    EXPECT_EQ(t.clock_type, GPR_TIMESPAN);
}

UTEST(DurationToTimespec, FromNegativeDuration) {
    gpr_timespec t = ugrpc::DurationToTimespec(engine::Deadline::Duration{-1});
    EXPECT_EQ(t.tv_sec, gpr_inf_past(GPR_TIMESPAN).tv_sec);
    EXPECT_EQ(t.tv_nsec, 0);
    EXPECT_EQ(t.clock_type, GPR_TIMESPAN);
}

UTEST(DurationToTimespec, FromMillisecondsMin) {
    gpr_timespec t = ugrpc::DurationToTimespec(std::chrono::milliseconds::min());
    EXPECT_EQ(t.tv_sec, gpr_inf_past(GPR_TIMESPAN).tv_sec);
    EXPECT_EQ(t.tv_nsec, 0);
    EXPECT_EQ(t.clock_type, GPR_TIMESPAN);
}

UTEST(DurationToTimespec, FromZeroDuration) {
    gpr_timespec t = ugrpc::DurationToTimespec(engine::Deadline::Duration::zero());
    EXPECT_EQ(t.tv_sec, 0);
    EXPECT_EQ(t.tv_nsec, 0);
    EXPECT_EQ(t.clock_type, GPR_TIMESPAN);
}

UTEST(DurationToTimespec, FromBaseTimeout) {
    const auto duration = kBaseTimeout;
    gpr_timespec t = ugrpc::DurationToTimespec(duration);
    EXPECT_EQ(t.tv_sec, 0);
    EXPECT_EQ(t.tv_nsec, std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count());
    EXPECT_EQ(t.clock_type, GPR_TIMESPAN);
}

UTEST(DurationToTimespec, FromLongTimeout) {
    const auto duration = engine::Deadline::Clock::now().time_since_epoch();
    const auto secs = std::chrono::floor<std::chrono::seconds>(duration);
    const auto nsecs = std::chrono::duration_cast<std::chrono::nanoseconds>(duration - secs);
    gpr_timespec t = ugrpc::DurationToTimespec(duration);
    EXPECT_EQ(t.tv_sec, secs.count());
    EXPECT_EQ(t.tv_nsec, nsecs.count());
    EXPECT_EQ(t.clock_type, GPR_TIMESPAN);
}

UTEST(TimespecToDuration, FromInfFuture) {
    EXPECT_EQ(ugrpc::TimespecToDuration(gpr_inf_future(GPR_CLOCK_MONOTONIC)), engine::Deadline::Duration::max());
}

UTEST(TimespecToDuration, FromInfPast) {
    EXPECT_EQ(ugrpc::TimespecToDuration(gpr_inf_past(GPR_CLOCK_MONOTONIC)), engine::Deadline::Duration::min());
}

UTEST(TimespecToDuration, FromNegative) {
    gpr_timespec t;
    t.tv_sec = -1;
    t.tv_nsec = GPR_NS_PER_SEC - 1;
    t.clock_type = GPR_TIMESPAN;
    EXPECT_EQ(ugrpc::TimespecToDuration(t), engine::Deadline::Duration::min());
}

UTEST(TimespecToDuration, FromBaseTimespec) {
    const auto duration = kBaseTimeout;
    const auto secs = std::chrono::floor<std::chrono::seconds>(duration);
    const auto nsecs = std::chrono::duration_cast<std::chrono::nanoseconds>(duration - secs);
    gpr_timespec t;
    t.tv_sec = secs.count();
    t.tv_nsec = nsecs.count();
    ASSERT_LT(t.tv_nsec, GPR_NS_PER_SEC);
    t.clock_type = GPR_TIMESPAN;
    EXPECT_EQ(ugrpc::TimespecToDuration(t), duration);
}

USERVER_NAMESPACE_END
