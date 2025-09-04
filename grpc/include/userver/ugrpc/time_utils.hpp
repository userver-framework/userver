#pragma once

#include <grpcpp/support/time.h>

#include <userver/engine/deadline.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc {

/// Converts `engine::Deadline::Duration` to `gpr_timespec`.
gpr_timespec DurationToTimespec(const engine::Deadline::Duration& duration) noexcept;

template <typename Rep, typename Period>
gpr_timespec DurationToTimespec(const std::chrono::duration<Rep, Period>& duration) noexcept {
    return DurationToTimespec(engine::Deadline::ToDurationSaturating(duration));
}

/// Converts `gpr_timespec` to `engine::Deadline::Duration`
engine::Deadline::Duration TimespecToDuration(gpr_timespec t) noexcept;

/// Converts `engine::Deadline` to `gpr_timespec`.
gpr_timespec DeadlineToTimespec(const engine::Deadline& deadline) noexcept;

/// Converts `gpr_timespec` to `engine::Deadline`
engine::Deadline TimespecToDeadline(gpr_timespec t) noexcept;

}  // namespace ugrpc

USERVER_NAMESPACE_END

template <>
class grpc::TimePoint<USERVER_NAMESPACE::engine::Deadline> {
public:
    /*implicit*/ TimePoint(const USERVER_NAMESPACE::engine::Deadline& time)
        : time_(USERVER_NAMESPACE::ugrpc::DeadlineToTimespec(time)) {}

    gpr_timespec raw_time() const noexcept { return time_; }

private:
    gpr_timespec time_;
};
