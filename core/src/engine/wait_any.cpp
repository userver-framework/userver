#include <userver/engine/wait_any.hpp>

#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

namespace {

class LockedWaitAnyStrategy final : public WaitStrategy {
 public:
  LockedWaitAnyStrategy(Deadline deadline,
                        IndexedWaitAnyElement* iwa_elements_begin,
                        IndexedWaitAnyElement* iwa_elements_end,
                        TaskContext& current)
      : WaitStrategy(deadline),
        iwa_elements_begin_(iwa_elements_begin),
        iwa_elements_end_(iwa_elements_end),
        current_(current) {}

  void SetupWakeups() override {
    bool wakeup_called = false;
    for (auto it = iwa_elements_begin_; it != iwa_elements_end_; ++it) {
      auto& [target, idx] = *it;
      target->AppendWaiter(current_);
      if (target->IsReady()) {
        if (!std::exchange(wakeup_called, true)) target->WakeupAllWaiters();
      }
    }
  }

  void DisableWakeups() override {
    for (auto it = iwa_elements_begin_; it != iwa_elements_end_; ++it) {
      auto& [target, idx] = *it;
      target->RemoveWaiter(current_);
    }
  }

 private:
  IndexedWaitAnyElement* const iwa_elements_begin_;
  IndexedWaitAnyElement* const iwa_elements_end_;
  TaskContext& current_;
};

bool AreUniqueValues(IndexedWaitAnyElement* iwa_elements_begin,
                     IndexedWaitAnyElement* iwa_elements_end) {
  std::sort(iwa_elements_begin, iwa_elements_end,
            [](const IndexedWaitAnyElement& x, const IndexedWaitAnyElement& y) {
              return x.context_accessor < y.context_accessor;
            });
  return std::adjacent_find(iwa_elements_begin, iwa_elements_end,
                            [](const IndexedWaitAnyElement& x,
                               const IndexedWaitAnyElement& y) {
                              return x.context_accessor == y.context_accessor;
                            }) == iwa_elements_end;
}

}  // namespace

std::optional<std::size_t> WaitAnyHelper::DoWaitAnyUntil(
    IndexedWaitAnyElement* iwa_elements_begin,
    IndexedWaitAnyElement* iwa_elements_end, Deadline deadline) {
  UASSERT(iwa_elements_begin <= iwa_elements_end);
  UASSERT_MSG(AreUniqueValues(iwa_elements_begin, iwa_elements_end),
              "Same tasks/futures were detected in WaitAny* call");
  if (iwa_elements_begin == iwa_elements_end) return std::nullopt;

  auto& current = current_task::GetCurrentTaskContext();
  for (auto it = iwa_elements_begin; it != iwa_elements_end; ++it) {
    auto& [wa_element, idx] = *it;
    if (wa_element->IsReady()) return idx;
  }

  if (current.ShouldCancel()) {
    throw WaitInterruptedException(current.CancellationReason());
  }
  LockedWaitAnyStrategy wait_manager(deadline, iwa_elements_begin,
                                     iwa_elements_end, current);
  current.Sleep(wait_manager);

  for (auto it = iwa_elements_begin; it != iwa_elements_end; ++it) {
    auto& [wa_element, idx] = *it;
    if (wa_element->IsReady()) return idx;
  }

  return std::nullopt;
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
