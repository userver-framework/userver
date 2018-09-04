#pragma once

/// @file engine/sleep.hpp
/// @brief Time-based coroutine suspension helpers

#include <chrono>

#include <engine/deadline.hpp>

namespace engine {

/// @brief Suspends execution for a brief period of time, possibly allowing
/// other tasks to execute
void Yield();

/// @cond
/// Recursion stopper/specialization
void SleepUntil(Deadline);
/// @endcond

/// Suspends execution for a specified amount of time
template <typename Rep, typename Period>
void SleepFor(const std::chrono::duration<Rep, Period>& duration) {
  SleepUntil(Deadline::FromDuration(duration));
}

/// Suspends execution until the specified time point is reached
template <typename Clock, typename Duration>
void SleepUntil(const std::chrono::time_point<Clock, Duration>& time_point) {
  SleepUntil(Deadline::FromTimePoint(time_point));
}

}  // namespace engine
