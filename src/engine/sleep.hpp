#pragma once

#include <chrono>

#include <engine/ev/timer.hpp>

#include "notifier.hpp"

namespace engine {

template <typename CurrentTask, typename Rep, typename Period>
void Sleep(std::chrono::duration<Rep, Period> duration) {
  auto notifier = CurrentTask::GetNotifier();
  ev::Timer<CurrentTask> timer(
      [notifier = std::move(notifier)] { notifier.Notify(); }, duration);
  CurrentTask::Wait();
}

}  // namespace engine
