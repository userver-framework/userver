#include <userver/engine/single_consumer_event.hpp>

#include <engine/impl/wait_list_light.hpp>
#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

class SingleConsumerEvent::EventWaitStrategy final : public impl::WaitStrategy {
 public:
  EventWaitStrategy(SingleConsumerEvent& event, impl::TaskContext& current,
                    Deadline deadline)
      : WaitStrategy(deadline), event_(event), current_(current) {}

  void SetupWakeups() override {
    event_.waiters_->Append(&current_);
    if (event_.is_signaled_.load()) {
      event_.waiters_->WakeupOne();
    }
  }

  void DisableWakeups() override { event_.waiters_->Remove(current_); }

 private:
  SingleConsumerEvent& event_;
  impl::TaskContext& current_;
};

SingleConsumerEvent::SingleConsumerEvent() noexcept = default;

SingleConsumerEvent::SingleConsumerEvent(NoAutoReset) noexcept
    : is_auto_reset_(false) {}

SingleConsumerEvent::~SingleConsumerEvent() = default;

bool SingleConsumerEvent::IsAutoReset() const noexcept {
  return is_auto_reset_;
}

bool SingleConsumerEvent::WaitForEvent() {
  return WaitForEventUntil(Deadline{});
}

bool SingleConsumerEvent::WaitForEventUntil(Deadline deadline) {
  if (GetIsSignaled()) {
    return true;  // optimistic path
  }

  impl::TaskContext& current = current_task::GetCurrentTaskContext();
  if (current.ShouldCancel()) return false;

  LOG_TRACE() << "WaitForEventUntil()";
  EventWaitStrategy wait_manager(*this, current, deadline);

  while (!GetIsSignaled()) {
    if (current.Sleep(wait_manager) !=
        impl::TaskContext::WakeupSource::kWaitList) {
      LOG_TRACE() << "cancelled";
      return false;
    }
  }

  LOG_TRACE() << "signaled";
  return true;
}

void SingleConsumerEvent::Reset() noexcept {
  is_signaled_.store(false, std::memory_order_seq_cst);
}

void SingleConsumerEvent::Send() {
  is_signaled_.store(true, std::memory_order_seq_cst);
  waiters_->WakeupOne();
}

bool SingleConsumerEvent::GetIsSignaled() noexcept {
  if (is_auto_reset_) {
    return is_signaled_.exchange(false);
  } else {
    return is_signaled_.load();
  }
}

}  // namespace engine

USERVER_NAMESPACE_END
