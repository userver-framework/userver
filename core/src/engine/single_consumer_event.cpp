#include <engine/single_consumer_event.hpp>
#include <engine/task/cancel.hpp>
#include "task/task_context.hpp"
#include "wait_list_light.hpp"

namespace engine {

namespace impl {
namespace {
class EventWaitStrategy final : public WaitStrategy {
 public:
  EventWaitStrategy(impl::WaitListLight& waiters,
                    const std::atomic<bool>& signaled, TaskContext* current,
                    Deadline deadline)
      : WaitStrategy(deadline),
        waiters_(waiters),
        is_signaled_(signaled),
        current_(current) {}

  void AfterAsleep() override {
    waiters_.Append(lock_, current_);
    if (is_signaled_) waiters_.WakeupOne(lock_);
  }

  void BeforeAwake() override { waiters_.Remove(lock_, current_); }

 private:
  impl::WaitListLight& waiters_;
  const std::atomic<bool>& is_signaled_;
  TaskContext* const current_;
  WaitListLight::Lock lock_;
};
}  // namespace
}  // namespace impl

SingleConsumerEvent::SingleConsumerEvent()
    : lock_waiters_(), is_signaled_(false) {}

SingleConsumerEvent::~SingleConsumerEvent() = default;

bool SingleConsumerEvent::WaitForEvent() {
  return WaitForEventUntil(Deadline{});
}

bool SingleConsumerEvent::WaitForEventUntil(Deadline deadline) {
  if (is_signaled_.exchange(false, std::memory_order_acquire)) {
    return true;  // optimistic path
  }

  impl::TaskContext* const current = current_task::GetCurrentTaskContext();
  if (current->ShouldCancel())
    return is_signaled_.exchange(false, std::memory_order_relaxed);

  LOG_TRACE() << "WaitForEvent()";
  impl::WaitListLight::SingleUserGuard guard(*lock_waiters_);
  impl::EventWaitStrategy wait_manager(*lock_waiters_, is_signaled_, current,
                                       deadline);

  bool was_signaled = false;
  while (!(was_signaled =
               is_signaled_.exchange(false, std::memory_order_relaxed)) &&
         !current->ShouldCancel()) {
    LOG_TRACE() << "iteration()";

    if (current->Sleep(wait_manager) ==
        impl::TaskContext::WakeupSource::kDeadlineTimer) {
      return false;
    }
  }
  LOG_TRACE() << "exit";

  return was_signaled;
}

void SingleConsumerEvent::Send() {
  is_signaled_.store(true, std::memory_order_release);

  impl::WaitListLight::Lock lock;
  lock_waiters_->WakeupOne(lock);
}

}  // namespace engine
