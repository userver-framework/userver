#include <engine/sleep.hpp>

#include "task/task_context.hpp"

namespace engine {

void InterruptibleSleepUntil(Deadline deadline) {
  auto context = current_task::GetCurrentTaskContext();

  impl::TaskContext::SleepParams new_sleep_params;
  new_sleep_params.deadline = std::move(deadline);
  context->Sleep(std::move(new_sleep_params));
}

void SleepUntil(Deadline deadline) {
  auto context = current_task::GetCurrentTaskContext();
  for (auto wakeup_source = impl::TaskContext::WakeupSource::kNone;
       wakeup_source != impl::TaskContext::WakeupSource::kDeadlineTimer;
       wakeup_source = context->GetWakeupSource()) {
    InterruptibleSleepUntil(deadline);
  }
}

void Yield() {
  SleepUntil(Deadline::FromTimePoint(Deadline::TimePoint::clock::now()));
}

}  // namespace engine
