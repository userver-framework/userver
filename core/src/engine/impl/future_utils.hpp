#pragma once

#include <type_traits>

#include <engine/task/task_context.hpp>
#include <userver/engine/future_status.hpp>
#include <userver/engine/impl/context_accessor.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

// Usable for future-like synchronization primitives that don't allow spurious
// wakeups and, once IsReady, are always IsReady.
template <typename T>
class FutureWaitStrategy final : public impl::WaitStrategy {
  static_assert(std::is_base_of_v<ContextAccessor, T>);

 public:
  FutureWaitStrategy(T& target, impl::TaskContext& current)
      : target_(target), current_(current) {}

  EarlyWakeup SetupWakeups() override {
    return target_.TryAppendWaiter(current_);
  }

  void DisableWakeups() noexcept override { target_.RemoveWaiter(current_); }

 private:
  T& target_;
  impl::TaskContext& current_;
};

inline FutureStatus ToFutureStatus(TaskContext::WakeupSource wakeup_source) {
  switch (wakeup_source) {
    case impl::TaskContext::WakeupSource::kCancelRequest:
      return FutureStatus::kCancelled;
    case impl::TaskContext::WakeupSource::kDeadlineTimer:
      return FutureStatus::kTimeout;
    case impl::TaskContext::WakeupSource::kWaitList:
      return FutureStatus::kReady;
    default:
      UINVARIANT(false, "Unexpected wakeup source");
  }
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
