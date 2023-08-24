#include <userver/engine/wait_all_checked.hpp>

#include <algorithm>

#include <engine/impl/wait_any_utils.hpp>
#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

FutureStatus DoWaitAllChecked(utils::impl::Span<ContextAccessor*> targets,
                              Deadline deadline) {
  UASSERT_MSG(AreUniqueValues(targets),
              "Same tasks/futures were detected in WaitAny* call");
  auto& current = current_task::GetCurrentTaskContext();

  WaitAnyWaitStrategy wait_strategy(deadline, targets, current);
  while (true) {
    bool all_completed = true;
    for (auto& target : targets) {
      if (!target) continue;

      const bool is_ready = target->IsReady();
      if (is_ready) {
        target->RethrowErrorResult();
        target = nullptr;
      }
      all_completed &= is_ready;
    }
    if (all_completed) break;

    switch (current.Sleep(wait_strategy)) {
      case TaskContext::WakeupSource::kWaitList:
        break;
      case TaskContext::WakeupSource::kDeadlineTimer:
        return FutureStatus::kTimeout;
      case TaskContext::WakeupSource::kCancelRequest:
        return FutureStatus::kCancelled;
      case TaskContext::WakeupSource::kNone:
        UASSERT_MSG(false, "Unexpected WakeupSource::kNone");
        break;
      case TaskContext::WakeupSource::kBootstrap:
        UASSERT_MSG(false, "Unexpected WakeupSource::kBootstrap");
        break;
    }
  }

  UASSERT(std::all_of(targets.begin(), targets.end(),
                      [](auto* target) { return !target; }));
  return FutureStatus::kReady;
}

void HandleWaitAllStatus(FutureStatus status) {
  switch (status) {
    case FutureStatus::kReady:
      break;
    case FutureStatus::kTimeout:
      UASSERT_MSG(false, "Got timeout on a WaitAll without Deadline");
      break;
    case FutureStatus::kCancelled:
      throw WaitInterruptedException(
          current_task::GetCurrentTaskContext().CancellationReason());
  }
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
