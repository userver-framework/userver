#pragma once

#include <chrono>

#include <engine/wait_helpers.hpp>

namespace engine {

void Yield();
void Sleep(Deadline);

template <typename Rep, typename Period>
void Sleep(const std::chrono::duration<Rep, Period>& duration) {
  Sleep(MakeDeadline(duration));
}

}  // namespace engine
