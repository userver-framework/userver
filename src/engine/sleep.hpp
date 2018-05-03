#pragma once

#include <chrono>

#include <engine/task/task.hpp>

#include "timer_event.hpp"

namespace engine {

template <typename Rep, typename Period>
void Sleep(const std::chrono::duration<Rep, Period>& duration) {
  auto& wake_up_cb = current_task::GetWakeUpCb();
  impl::TimerEvent timer([&wake_up_cb] { wake_up_cb(); }, duration);
  current_task::Wait();
}

}  // namespace engine
