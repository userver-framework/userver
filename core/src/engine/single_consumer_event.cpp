#include <userver/engine/single_consumer_event.hpp>

#include <userver/engine/task/cancel.hpp>

#include <engine/impl/wait_list_light.hpp>
#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace {

class EventWaitStrategy final : public impl::WaitStrategy {
 public:
  EventWaitStrategy(impl::WaitListLight& waiters,
                    const std::atomic<bool>& signaled,
                    impl::TaskContext& current, Deadline deadline)
      : WaitStrategy(deadline),
        waiters_(waiters),
        is_signaled_(signaled),
        current_(current) {}

  void SetupWakeups() override {
    waiters_.Append(&current_);
    if (is_signaled_) waiters_.WakeupOne();
  }

  void DisableWakeups() override { waiters_.Remove(current_); }

 private:
  impl::WaitListLight& waiters_;
  const std::atomic<bool>& is_signaled_;
  impl::TaskContext& current_;
};

}  // namespace

SingleConsumerEvent::SingleConsumerEvent() = default;

SingleConsumerEvent::SingleConsumerEvent(NoAutoReset) : is_auto_reset_(false) {}

SingleConsumerEvent::~SingleConsumerEvent() = default;

bool SingleConsumerEvent::IsAutoReset() const { return is_auto_reset_; }

bool SingleConsumerEvent::WaitForEvent() {
  return WaitForEventUntil(Deadline{});
}

bool SingleConsumerEvent::WaitForEventUntil(Deadline deadline) {
  if (GetIsSignaled()) {
    return true;  // optimistic path
  }

  impl::TaskContext& current = current_task::GetCurrentTaskContext();
  if (current.ShouldCancel()) return GetIsSignaled();

  LOG_TRACE() << "WaitForEvent()";
  impl::WaitListLight::SingleUserGuard guard(*lock_waiters_);
  EventWaitStrategy wait_manager(*lock_waiters_, is_signaled_, current,
                                 deadline);

  bool was_signaled = false;
  while (!(was_signaled = GetIsSignaled()) && !current.ShouldCancel()) {
    LOG_TRACE() << "iteration()";

    if (current.Sleep(wait_manager) ==
        impl::TaskContext::WakeupSource::kDeadlineTimer) {
      return false;
    }
  }
  LOG_TRACE() << "exit";

  return was_signaled;
}

void SingleConsumerEvent::Reset() { is_signaled_ = false; }

void SingleConsumerEvent::Send() {
  is_signaled_.store(true, std::memory_order_release);

  lock_waiters_->WakeupOne();
}

bool SingleConsumerEvent::GetIsSignaled() {
  if (is_auto_reset_) {
    return is_signaled_.exchange(false);
  } else {
    return is_signaled_.load();
  }
}

}  // namespace engine

USERVER_NAMESPACE_END
