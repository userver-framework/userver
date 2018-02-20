#pragma once

#include <chrono>

#include <engine/ev/timer.hpp>
#include <engine/task/task.hpp>

#include "notifier.hpp"

namespace engine {

template <typename Rep, typename Period>
void Sleep(const std::chrono::duration<Rep, Period>& duration) {
  auto& notifier = CurrentTask::GetNotifier();
  ev::Timer<CurrentTask> timer([&notifier] { notifier.Notify(); }, duration);
  CurrentTask::Wait();
}

}  // namespace engine
