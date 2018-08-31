#pragma once

/// @file engine/deadline.hpp
/// @brief Internal representation of a deadline time point

#include <chrono>

namespace engine {

/// Internal representation of a deadline time point
using Deadline = std::chrono::steady_clock::time_point;

/// Converts duration to a Deadline
template <typename Rep, typename Period>
Deadline MakeDeadline(const std::chrono::duration<Rep, Period>& duration) {
  return Deadline::clock::now() + duration;
}

/// Converts time point to a Deadline
template <typename Clock, typename Duration>
Deadline MakeDeadline(
    const std::chrono::time_point<Clock, Duration>& time_point) {
  return MakeDeadline(time_point - Clock::now());
}

/// @cond
/// Recursion stopper/specialization
inline Deadline MakeDeadline(const Deadline& deadline) { return deadline; }
/// @endcond

}  // namespace engine
