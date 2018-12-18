#include <engine/sleep.hpp>

#include <engine/task/cancel.hpp>

#include <engine/task/task_context.hpp>

namespace engine {

void InterruptibleSleepUntil(Deadline deadline) {
  auto context = current_task::GetCurrentTaskContext();

  impl::TaskContext::SleepParams new_sleep_params;
  new_sleep_params.deadline = std::move(deadline);
  context->Sleep(std::move(new_sleep_params));
}

void SleepUntil(Deadline deadline) {
  TaskCancellationBlocker block_cancel;
  InterruptibleSleepUntil(deadline);
}

void Yield() {
  SleepUntil(Deadline::FromTimePoint(Deadline::TimePoint::clock::now()));
}

}  // namespace engine
