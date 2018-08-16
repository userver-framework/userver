#pragma once

#include <chrono>

namespace engine {

using Deadline = std::chrono::steady_clock::time_point;

template <typename Rep, typename Period>
Deadline MakeDeadline(const std::chrono::duration<Rep, Period>& duration) {
  return Deadline::clock::now() + duration;
}

template <typename Clock, typename Duration>
Deadline MakeDeadline(
    const std::chrono::time_point<Clock, Duration>& time_point) {
  return MakeDeadline(time_point - Clock::now());
}

inline Deadline MakeDeadline(const Deadline& deadline) { return deadline; }

}  // namespace engine
