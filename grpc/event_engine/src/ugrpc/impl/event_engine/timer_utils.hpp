#pragma once

#include <algorithm>
#include <chrono>
#include <type_traits>

#include <userver/engine/deadline.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::impl::event_engine {

using TimerClock = std::chrono::steady_clock;

inline TimerClock::time_point MakeTimerDeadline(TimerClock::time_point now, std::chrono::nanoseconds when) noexcept {
    const auto delay = engine::Deadline::ToDurationSaturating(std::max(when, std::chrono::nanoseconds::zero()));
    if (now.time_since_epoch() > TimerClock::duration::max() - delay) {
        return TimerClock::time_point::max();
    }
    return now + delay;
}

inline double GetTimerDelay(TimerClock::time_point deadline, TimerClock::time_point now) noexcept {
    if (deadline <= now) {
        return 0.0;
    }
    using UnsignedRep = std::make_unsigned_t<TimerClock::rep>;
    using UnsignedDuration = std::chrono::duration<UnsignedRep, TimerClock::period>;
    const auto ticks =
        static_cast<UnsignedRep>(deadline.time_since_epoch().count()) -
        static_cast<UnsignedRep>(now.time_since_epoch().count());
    return std::chrono::duration<double>(UnsignedDuration{ticks}).count();
}

}  // namespace ugrpc::impl::event_engine

USERVER_NAMESPACE_END
