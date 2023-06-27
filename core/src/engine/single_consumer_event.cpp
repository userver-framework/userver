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
    if (event_.waiters_->GetSignalOrAppend(&current_)) {
      current_.WakeupCurrent();
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
  LOG_TRACE() << "WaitForEventUntil()";
  EventWaitStrategy wait_manager(*this, current, deadline);

  while (true) {
    if (GetIsSignaled()) {
      LOG_TRACE() << "success";
      return true;
    }

    LOG_TRACE() << "iteration()";

    const auto wakeup_source = current.Sleep(wait_manager);
    if (!impl::HasWaitSucceeded(wakeup_source)) {
      LOG_TRACE() << "failure";
      return false;
    }
  }
}

void SingleConsumerEvent::Reset() noexcept { waiters_->GetAndResetSignal(); }

void SingleConsumerEvent::Send() { waiters_->SetSignalAndWakeupOne(); }

bool SingleConsumerEvent::IsReady() const noexcept {
  return waiters_->IsSignaled();
}

bool SingleConsumerEvent::GetIsSignaled() noexcept {
  if (is_auto_reset_) {
    return waiters_->GetAndResetSignal();
  } else {
    return waiters_->IsSignaled();
  }
}

}  // namespace engine

USERVER_NAMESPACE_END
