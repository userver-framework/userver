#pragma once

#include <chrono>

#include <engine/deadline.hpp>

namespace engine {

void Yield();
void SleepUntil(Deadline);

template <typename Rep, typename Period>
void SleepFor(const std::chrono::duration<Rep, Period>& duration) {
  SleepUntil(MakeDeadline(duration));
}

template <typename Clock, typename Duration>
void SleepUntil(const std::chrono::time_point<Clock, Duration>& deadline) {
  SleepUntil(MakeDeadline(deadline));
}

}  // namespace engine
