#pragma once

/// @file userver/engine/deadline.hpp
/// @brief Internal representation of a deadline time point

#include <chrono>

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

  /// Returns the duration of time left before the reachable deadline
  Duration TimeLeft() const noexcept;

  /// Returns the approximate duration of time left before the reachable
  /// deadline. May be faster than TimeLeft.
  /// @see utils::datetime::SteadyCoarseClock
  Duration TimeLeftApprox() const noexcept;

  /// Converts duration to a Deadline
  template <typename Rep, typename Period>
  static Deadline FromDuration(
      const std::chrono::duration<Rep, Period>& duration) noexcept {
    return Deadline(TimePoint::clock::now() + duration);
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

  static constexpr TimePoint kPassed = TimePoint::min();

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

  TimePoint value_;
};

}  // namespace engine

USERVER_NAMESPACE_END
