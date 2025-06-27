#pragma once

/// @file userver/engine/deadline.hpp
/// @brief Internal representation of a deadline time point

#include <chrono>
#include <type_traits>

USERVER_NAMESPACE_BEGIN

namespace engine {

/// @brief Internal representation of a deadline time point
class Deadline final {
public:
    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;
    using Duration = TimePoint::duration;

    /// Creates an unreachable deadline
    constexpr Deadline() = default;

    /// Returns whether the deadline can be reached
    constexpr bool IsReachable() const noexcept { return value_ != kUnreachable; }

    /// Returns whether the deadline is reached
    bool IsReached() const noexcept;

    /// Returns whether the deadline is reached. Will report false-negatives, will
    /// never report false-positives.
    bool IsSurelyReachedApprox() const noexcept;

    /// Returns the duration of time left before the reachable deadline
    Duration TimeLeft() const noexcept;

    /// Returns the approximate duration of time left before the reachable
    /// deadline. May be faster than TimeLeft.
    /// @see utils::datetime::SteadyCoarseClock
    Duration TimeLeftApprox() const noexcept;

    /// Returns the native time point value.
    /// Returns `TimePoint::max()` for unreachable deadline
    /// and `TimePoint::min()` for `Deadline::Passed()`
    constexpr TimePoint GetTimePoint() const noexcept { return value_; }

    /// Converts duration to a Deadline
    template <typename Rep, typename Period>
    static Deadline FromDuration(const std::chrono::duration<Rep, Period>& incoming_duration) noexcept {
        using IncomingDuration = std::chrono::duration<Rep, Period>;

        // Require resolution 'IncomingDuration' <= 'Duration',
        // so it is safe casting 'Duration::max' to 'IncomingDuration'

        static_assert(std::is_constructible_v<Duration, IncomingDuration>);

        if (std::chrono::duration_cast<IncomingDuration>(Duration::max()) <= incoming_duration) {
            return Deadline{};
        }

        if (incoming_duration < IncomingDuration::zero()) {
            return Deadline::Passed();
        }

        return Deadline{SumWithSaturation(Clock::now(), std::chrono::duration_cast<Duration>(incoming_duration))};
    }

    /// @brief Converts time point to a Deadline
    ///
    /// Non-steady clocks may produce inaccurate Deadlines. Prefer using
    /// Deadline::FromDuration or std::chrono::steady_clock::time_point
    /// if possible.
    template <typename Clock, typename Duration>
    static Deadline FromTimePoint(const std::chrono::time_point<Clock, Duration>& time_point) noexcept {
        return FromDuration(time_point - Clock::now());
    }

    /// @cond
    /// Specialization for the native time point type
    constexpr static Deadline FromTimePoint(const TimePoint& time_point) noexcept { return Deadline(time_point); }
    /// @endcond

    /// A Deadline that is guaranteed to be IsReached
    constexpr static Deadline Passed() noexcept { return Deadline{kPassed}; }

    constexpr bool operator==(const Deadline& r) const noexcept { return value_ == r.value_; }

    constexpr bool operator<(const Deadline& r) const noexcept {
        if (!IsReachable()) return false;
        if (!r.IsReachable()) return true;
        return value_ < r.value_;
    }

private:
    constexpr explicit Deadline(TimePoint value) noexcept : value_(value) {}

    constexpr static TimePoint SumWithSaturation(const TimePoint& time_point, Duration duration) noexcept {
        return TimePoint::max() - time_point < duration ? TimePoint::max() : time_point + duration;
    }

    static constexpr TimePoint kUnreachable = TimePoint::max();
    static constexpr TimePoint kPassed = TimePoint::min();

    TimePoint value_{kUnreachable};
};

}  // namespace engine

USERVER_NAMESPACE_END
