#pragma once

/// @file userver/engine/deadline.hpp
/// @brief Internal representation of a deadline time point

#include <chrono>
#include <type_traits>

#include <userver/utils/assert.hpp>

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
  constexpr bool IsReachable() const noexcept { return value_ != TimePoint{}; }

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

  /// Converts duration to a Deadline
  template <typename Rep, typename Period>
  static Deadline FromDuration(
      const std::chrono::duration<Rep, Period>& incoming_duration) noexcept {
    using IncomingDuration = std::chrono::duration<Rep, Period>;

    if (incoming_duration.count() < 0) {
      return Deadline::Passed();
    }

    const auto now = TimePoint::clock::now();
    constexpr auto max_now = TimePoint::clock::time_point::max();

    // If:
    // 1. incoming_duration would overflow Duration,
    // 2. or adding it to 'now' would overflow,
    // then set deadline to unreachable right away.

    // Implementation strategy:
    // 1. Check that resolution of Duration >= that of IncomingDuration.
    static_assert(std::is_constructible_v<Duration, IncomingDuration>);

    // 2. As it is higher, then the range is lower (or equal). So casting
    //    Duration::max to IncomingDuration is safe. Do a quick check
    //    that Duration{incoming_duration} won't overflow.
    if (incoming_duration >
        std::chrono::duration_cast<IncomingDuration>(Duration::max())) {
      OnDurationOverflow(
          std::chrono::duration_cast<std::chrono::duration<double>>(
              incoming_duration));
      return Deadline{};
    }

    // 3. Check that now + Duration{incoming_duration} won't overflow.
    UASSERT(max_now - now >= Duration{incoming_duration});

    return Deadline(now + Duration{incoming_duration});
  }

  /// @brief Converts time point to a Deadline
  ///
  /// Non-steady clocks may produce inaccurate Deadlines. Prefer using
  /// Deadline::FromDuration or std::chrono::steady_clock::time_point
  /// if possible.
  template <typename Clock, typename Duration>
  static Deadline FromTimePoint(
      const std::chrono::time_point<Clock, Duration>& time_point) noexcept {
    return FromDuration(time_point - Clock::now());
  }

  /// @cond
  /// Specialization for the native time point type
  constexpr static Deadline FromTimePoint(const TimePoint& time_point) {
    return Deadline(time_point);
  }
  /// @endcond

  /// A Deadline that is guaranteed to be IsReached
  constexpr static Deadline Passed() noexcept { return Deadline{kPassed}; }

  constexpr bool operator==(const Deadline& r) const noexcept {
    return value_ == r.value_;
  }

  constexpr bool operator<(const Deadline& r) const noexcept {
    if (!IsReachable()) return false;
    if (!r.IsReachable()) return true;
    return value_ < r.value_;
  }

 private:
  constexpr explicit Deadline(TimePoint value) noexcept : value_(value) {}

  static void OnDurationOverflow(
      std::chrono::duration<double> incoming_duration);

  static constexpr TimePoint kPassed = TimePoint::min();

  TimePoint value_;
};

}  // namespace engine

USERVER_NAMESPACE_END
