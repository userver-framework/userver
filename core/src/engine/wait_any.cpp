#include <userver/engine/wait_any.hpp>

#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class LockedWaitAnyStrategy final : public WaitStrategy {
 public:
  LockedWaitAnyStrategy(Deadline deadline,
                        std::vector<IndexedWaitAnyElement>& targets,
                        TaskContext& current)
      : WaitStrategy(deadline), targets_(targets), current_(current) {}

  void SetupWakeups() override {
    bool wakeup_called = false;
    for (auto& [target, idx] : targets_) {
      target.AppendWaiter(&current_);
      if (target.IsReady()) {
        if (!std::exchange(wakeup_called, true)) target.WakeupOneWaiter();
      }
    }
  }

  void DisableWakeups() override {
    for (auto& [target, idx] : targets_) {
      target.RemoveWaiter(&current_);
    }
  }

 private:
  std::vector<IndexedWaitAnyElement>& targets_;
  TaskContext& current_;
};

std::optional<size_t> WaitAnyHelper::DoWaitAnyUntil(
    std::vector<IndexedWaitAnyElement>& iwa_elements, Deadline deadline) {
  if (iwa_elements.empty()) return std::nullopt;

  auto current = current_task::GetCurrentTaskContext();
  for (auto& [wa_element, idx] : iwa_elements) {
    if (wa_element.IsReady()) return idx;
    if (!wa_element.IsWaitingEnabledFrom(current)) ReportDeadlock();
  }

  if (current->ShouldCancel()) {
    throw WaitInterruptedException(current->CancellationReason());
  }
  LockedWaitAnyStrategy wait_manager(deadline, iwa_elements, *current);
  current->Sleep(wait_manager);

  for (auto& [wa_element, idx] : iwa_elements) {
    if (wa_element.IsReady()) return idx;
  }

  return std::nullopt;
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
