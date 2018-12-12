#include <engine/single_consumer_event.hpp>
#include <engine/task/cancel.hpp>
#include "task/task_context.hpp"
#include "wait_list_light.hpp"

namespace engine {

SingleConsumerEvent::SingleConsumerEvent()
    : lock_waiters_(std::make_shared<impl::WaitListLight>()),
      signaled_(false) {}

SingleConsumerEvent::~SingleConsumerEvent() = default;

bool SingleConsumerEvent::WaitForEvent() {
  bool was_signaled = false;
  if (current_task::ShouldCancel()) return was_signaled;

  lock_waiters_->PinToCurrentTask();

  impl::TaskContext* const current = current_task::GetCurrentTaskContext();
  LOG_TRACE() << "WaitForEvent()";

  impl::WaitListLight::Lock lock;

  while (!(was_signaled = signaled_.exchange(false)) &&
         !current_task::ShouldCancel()) {
    LOG_TRACE() << "iteration()";

    impl::TaskContext::SleepParams sleep_params;

    sleep_params.wait_list = lock_waiters_;
    sleep_params.exec_after_asleep = [this, &lock, current] {
      LOG_TRACE() << "exec_after_asleep()";
      lock_waiters_->Append(lock, current);
      if (signaled_) lock_waiters_->WakeupOne(lock);
    };

    current->Sleep(std::move(sleep_params));
  }
  LOG_TRACE() << "exit";

  return was_signaled;
}

void SingleConsumerEvent::Send() {
  signaled_ = true;

  impl::WaitListLight::Lock lock;
  lock_waiters_->WakeupOne(lock);
}

}  // namespace engine
