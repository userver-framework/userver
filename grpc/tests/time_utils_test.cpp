#include <gmock/gmock.h>
#include <userver/utest/utest.hpp>

#include <grpc/support/time.h>

#include <userver/engine/sleep.hpp>

#include <userver/ugrpc/time_utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr auto kBaseTimeout = std::chrono::milliseconds{200};

}  // namespace

UTEST(DurationToTimespec, FromDurationMax) {
    EXPECT_EQ(
        ugrpc::DurationToTimespec(engine::Deadline::Duration::max()).tv_sec, gpr_inf_future(GPR_CLOCK_MONOTONIC).tv_sec
    );
}

UTEST(DurationToTimespec, FromNegativeDuration) {
    EXPECT_EQ(
        ugrpc::DurationToTimespec(engine::Deadline::Duration{-1}).tv_sec, gpr_inf_past(GPR_CLOCK_MONOTONIC).tv_sec
    );
}

UTEST(DurationToTimespec, FromZeroDuration) {
    gpr_timespec low = gpr_now(GPR_CLOCK_MONOTONIC);
    gpr_timespec t = ugrpc::DurationToTimespec(engine::Deadline::Duration::zero());
    gpr_timespec high = gpr_now(GPR_CLOCK_MONOTONIC);
    EXPECT_LE(low.tv_sec, t.tv_sec);
    EXPECT_LE(t.tv_sec, high.tv_sec);
}

UTEST(DurationToTimespec, FromBaseTimeout) {
    const auto duration = kBaseTimeout;
    const auto nsecs = std::chrono::duration_cast<std::chrono::nanoseconds>(kBaseTimeout);
    gpr_timespec low = gpr_now(GPR_CLOCK_MONOTONIC);
    low.tv_sec += (low.tv_nsec + nsecs.count()) / GPR_NS_PER_SEC;
    gpr_timespec t = ugrpc::DurationToTimespec(duration);
    gpr_timespec high = gpr_now(GPR_CLOCK_MONOTONIC);
    high.tv_sec += (high.tv_nsec + nsecs.count()) / GPR_NS_PER_SEC;
    EXPECT_LE(low.tv_sec, t.tv_sec);
    EXPECT_LE(t.tv_sec, high.tv_sec);
}

UTEST(DurationToTimespec, FromLongTimeout) {
    const auto duration = engine::Deadline::Clock::now().time_since_epoch();
    const auto secs = std::chrono::floor<std::chrono::seconds>(duration);
    const auto nsecs = std::chrono::duration_cast<std::chrono::nanoseconds>(duration - secs);
    gpr_timespec low = gpr_now(GPR_CLOCK_MONOTONIC);
    low.tv_sec += secs.count();
    low.tv_sec += (low.tv_nsec + nsecs.count()) / GPR_NS_PER_SEC;
    gpr_timespec t = ugrpc::DurationToTimespec(duration);
    gpr_timespec high = gpr_now(GPR_CLOCK_MONOTONIC);
    high.tv_sec += secs.count();
    high.tv_sec += (high.tv_nsec + nsecs.count()) / GPR_NS_PER_SEC;
    EXPECT_LE(low.tv_sec, t.tv_sec);
    EXPECT_LE(t.tv_sec, high.tv_sec);
}

UTEST(TimespecToDuration, FromInfFuture) {
    EXPECT_EQ(ugrpc::TimespecToDuration(gpr_inf_future(GPR_CLOCK_MONOTONIC)), engine::Deadline::Duration::max());
}

UTEST(TimespecToDuration, FromInfinity) {
    EXPECT_EQ(ugrpc::TimespecToDuration(gpr_inf_future(GPR_CLOCK_MONOTONIC)), engine::Deadline::Duration::max());
}

UTEST(TimespecToDuration, FromNegative) {
    gpr_timespec t = gpr_now(GPR_CLOCK_MONOTONIC);
    --t.tv_nsec;
    if (t.tv_nsec < 0) {
        t.tv_nsec += GPR_NS_PER_SEC;
        t.tv_sec--;
    }
    EXPECT_EQ(ugrpc::TimespecToDuration(t), engine::Deadline::Duration::min());
}

UTEST(TimespecToDuration, FromBaseTimespec) {
    const auto duration = kBaseTimeout;
    const auto secs = std::chrono::floor<std::chrono::seconds>(duration);
    const auto nsecs = std::chrono::duration_cast<std::chrono::nanoseconds>(duration - secs);
    gpr_timespec t = gpr_now(GPR_CLOCK_MONOTONIC);
    t.tv_sec += secs.count();
    t.tv_nsec += nsecs.count();
    if (t.tv_nsec >= GPR_NS_PER_SEC) {
        t.tv_nsec -= GPR_NS_PER_SEC;
        t.tv_sec++;
    }
    EXPECT_THAT(
        ugrpc::TimespecToDuration(t),
        testing::AllOf(testing::Ge(engine::Deadline::Duration::zero()), testing::Le(duration))
    );
}

UTEST(TimeUtils, BaseTimeout) {
    const auto duration = kBaseTimeout;
    gpr_timespec t = ugrpc::DurationToTimespec(duration);
    EXPECT_THAT(
        ugrpc::TimespecToDuration(t),
        testing::AllOf(testing::Ge(engine::Deadline::Duration::zero()), testing::Le(duration))
    );
}

UTEST(TimeUtils, Passed) {
    const auto duration = kBaseTimeout;
    gpr_timespec t = ugrpc::DurationToTimespec(duration);
    engine::InterruptibleSleepFor(duration);
    EXPECT_EQ(ugrpc::TimespecToDuration(t), engine::Deadline::Duration::min());
}

USERVER_NAMESPACE_END
