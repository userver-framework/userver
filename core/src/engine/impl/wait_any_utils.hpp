#pragma once

#include <algorithm>
#include <utility>
#include <vector>

#include <engine/task/task_context.hpp>
#include <userver/engine/impl/context_accessor.hpp>
#include <userver/utils/impl/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class WaitAnyWaitStrategy final : public WaitStrategy {
 public:
  WaitAnyWaitStrategy(Deadline deadline,
                      utils::impl::Span<ContextAccessor*> targets,
                      TaskContext& current)
      : WaitStrategy(deadline), current_(current), targets_(targets) {}

  void SetupWakeups() override {
    for (auto& target : targets_) {
      if (!target) continue;

      target->AppendWaiter(current_);
      if (target->IsReady()) {
        active_targets_ = {targets_.begin(), &target + 1};
        current_.Wakeup(TaskContext::WakeupSource::kWaitList,
                        TaskContext::NoEpoch{});
        return;
      }
    }

    active_targets_ = targets_;
  }

  void DisableWakeups() override {
    for (auto& target : active_targets_) {
      if (!target) continue;

      target->RemoveWaiter(current_);
    }
  }

 private:
  TaskContext& current_;
  const utils::impl::Span<ContextAccessor*> targets_;
  utils::impl::Span<ContextAccessor*> active_targets_;
};

inline bool AreUniqueValues(utils::impl::Span<ContextAccessor*> targets) {
  std::vector<ContextAccessor*> sorted;
  sorted.reserve(targets.size());
  std::copy_if(targets.begin(), targets.end(), std::back_inserter(sorted),
               [](const auto& target) { return target != nullptr; });
  std::sort(sorted.begin(), sorted.end());
  return std::adjacent_find(sorted.begin(), sorted.end()) == sorted.end();
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
