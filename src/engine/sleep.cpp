#include <engine/sleep.hpp>

#include "task/task_context.hpp"

namespace engine {

void SleepUntil(Deadline deadline) {
  auto context = current_task::GetCurrentTaskContext();

  impl::TaskContext::SleepParams new_sleep_params;
  new_sleep_params.deadline = std::move(deadline);
  context->Sleep(std::move(new_sleep_params));
}

void Yield() { SleepUntil(Deadline::clock::now()); }

}  // namespace engine
