#pragma once

#include <algorithm>
#include <utility>
#include <vector>

#include <engine/task/task_context.hpp>
#include <userver/engine/impl/context_accessor.hpp>
#include <userver/utils/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class WaitAnyWaitStrategy final : public WaitStrategy {
 public:
  WaitAnyWaitStrategy(Deadline deadline, utils::span<ContextAccessor*> targets,
                      TaskContext& current)
      : WaitStrategy(deadline), current_(current), targets_(targets) {}

  void SetupWakeups() override {
    for (auto& target : targets_) {
      if (!target) continue;

      target->AppendWaiter(current_);
      if (target->IsReady()) {
        active_targets_ = {targets_.data(), &target + 1};
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
  const utils::span<ContextAccessor*> targets_;
  utils::span<ContextAccessor*> active_targets_;
};

inline bool AreUniqueValues(utils::span<ContextAccessor*> targets) {
  std::vector<ContextAccessor*> sorted;
  sorted.reserve(targets.size());
  std::copy_if(targets.begin(), targets.end(), std::back_inserter(sorted),
               [](const auto& target) { return target != nullptr; });
  std::sort(sorted.begin(), sorted.end());
  return std::adjacent_find(sorted.begin(), sorted.end()) == sorted.end();
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
