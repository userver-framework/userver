#include <userver/ugrpc/time_utils.hpp>

#include <chrono>
#include <cstdint>

#include <grpc/support/time.h>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc {

gpr_timespec DurationToTimespec(const engine::Deadline::Duration& duration) noexcept {
    const auto secs = std::chrono::floor<std::chrono::seconds>(duration);
    if (duration == engine::Deadline::Duration::max() || secs.count() >= gpr_inf_future(GPR_CLOCK_MONOTONIC).tv_sec) {
        return gpr_inf_future(GPR_TIMESPAN);
    }
    if (secs.count() < 0) {
        return gpr_inf_past(GPR_TIMESPAN);
    }
    const auto nsecs = std::chrono::duration_cast<std::chrono::nanoseconds>(duration - secs);
    UASSERT(0 <= nsecs.count() && nsecs.count() < GPR_NS_PER_SEC);
    gpr_timespec t;
    t.tv_sec = static_cast<std::int64_t>(secs.count());
    t.tv_nsec = static_cast<std::int32_t>(nsecs.count());
    t.clock_type = GPR_TIMESPAN;
    return t;
}

engine::Deadline::Duration TimespecToDuration(gpr_timespec t) noexcept {
    t = gpr_convert_clock_type(t, GPR_TIMESPAN);
    if (gpr_time_cmp(t, gpr_inf_future(t.clock_type)) == 0 ||
        t.tv_sec >= std::chrono::duration_cast<std::chrono::seconds>(engine::Deadline::Duration::max()).count()) {
        return engine::Deadline::Duration::max();
    }
    if (gpr_time_cmp(t, gpr_time_0(t.clock_type)) < 0) {
        return engine::Deadline::Duration::min();
    }
    const auto secs = std::chrono::seconds{t.tv_sec};
    UASSERT(0 <= t.tv_nsec && t.tv_nsec < GPR_NS_PER_SEC);
    const auto nsecs = std::chrono::nanoseconds{t.tv_nsec};
    return std::chrono::duration_cast<engine::Deadline::Duration>(secs) +
           std::chrono::duration_cast<engine::Deadline::Duration>(nsecs);
}

gpr_timespec DeadlineToTimespec(const engine::Deadline& deadline) noexcept {
    gpr_timespec t =
        DurationToTimespec(deadline.IsReachable() ? deadline.TimeLeft() : engine::Deadline::Duration::max());
    return gpr_convert_clock_type(t, GPR_CLOCK_MONOTONIC);
}

engine::Deadline TimespecToDeadline(gpr_timespec t) noexcept {
    return engine::Deadline::FromDuration(TimespecToDuration(t));
}

}  // namespace ugrpc

USERVER_NAMESPACE_END
