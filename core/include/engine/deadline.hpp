#pragma once

/// @file engine/deadline.hpp
/// @brief Internal representation of a deadline time point

#include <chrono>
#include <utility>

#include <utils/assert.hpp>

namespace engine {

/// Internal representation of a deadline time point
class Deadline final {
 public:
  using Clock = std::chrono::steady_clock;
  using TimePoint = Clock::time_point;

  /// Creates an unreachable deadline
  constexpr Deadline() = default;

  /// Returns whether the deadline can be reached
  constexpr bool IsReachable() const { return value_ != TimePoint{}; }

  /// Returns whether the deadline is reached
  bool IsReached() const {
    if (!IsReachable()) return false;
    return value_ == kPassed || value_ < TimePoint::clock::now();
  }

  /// Returns the duration of time left before the reachable deadline
  auto TimeLeft() const {
    UASSERT(IsReachable());
    if (value_ == kPassed) return Clock::duration::zero();
    return value_ - TimePoint::clock::now();
  }

  /// Converts duration to a Deadline
  template <typename Rep, typename Period>
  static Deadline FromDuration(
      const std::chrono::duration<Rep, Period>& duration) {
    return Deadline(TimePoint::clock::now() + duration);
  }

  /// Converts time point to a Deadline
  template <typename Clock, typename Duration>
  static Deadline FromTimePoint(
      const std::chrono::time_point<Clock, Duration>& time_point) {
    return FromDuration(time_point - Clock::now());
  }

  /// @cond
  /// Specialization for the native time point type
  constexpr static Deadline FromTimePoint(const TimePoint& time_point) {
    return Deadline(time_point);
  }
  /// @endcond

  static constexpr TimePoint kPassed = TimePoint::min();

  constexpr static Deadline Passed() { return Deadline{kPassed}; }

  constexpr bool operator==(const Deadline& r) const {
    return value_ == r.value_;
  }

 private:
  constexpr explicit Deadline(TimePoint value) : value_(value) {}

  TimePoint value_;
};

}  // namespace engine
