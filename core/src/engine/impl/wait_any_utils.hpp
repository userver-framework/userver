#pragma once

#include <algorithm>
#include <utility>
#include <vector>

#include <engine/task/task_context.hpp>
#include <userver/engine/impl/context_accessor.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class WaitAnyWaitStrategy final : public WaitStrategy {
 public:
  WaitAnyWaitStrategy(utils::span<ContextAccessor*> targets,
                      TaskContext& waiter)
      : waiter_(waiter), targets_(targets) {}

  EarlyWakeup SetupWakeups() override {
    for (auto*& target : targets_) {
      if (!target) continue;

      utils::FastScopeGuard disable_wakeups([&]() noexcept {
        DoDisableWakeups(utils::span{targets_.data(), &target});
      });

      // SetupWakeups might throw.
      const auto early_wakeup = target->TryAppendWaiter(waiter_);

      if (static_cast<bool>(early_wakeup)) {
        return EarlyWakeup{true};
      }

      disable_wakeups.Release();
    }

    return EarlyWakeup{false};
  }

  void DisableWakeups() noexcept override { DoDisableWakeups(targets_); }

 private:
  void DoDisableWakeups(utils::span<ContextAccessor*> targets) const noexcept {
    for (auto* const target : targets) {
      if (!target) continue;
      target->RemoveWaiter(waiter_);
    }
  }

  TaskContext& waiter_;
  const utils::span<ContextAccessor*> targets_;
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
