#include <engine/single_consumer_event.hpp>
#include "task/task_context.hpp"
#include "wait_list_light.hpp"

namespace engine {

SingleConsumerEvent::SingleConsumerEvent()
    : lock_waiters_(std::make_shared<impl::WaitListLight>()),
      signaled_(false) {}

SingleConsumerEvent::~SingleConsumerEvent() = default;

void SingleConsumerEvent::WaitForEvent() {
  lock_waiters_->PinToCurrentTask();

  impl::TaskContext* const current = current_task::GetCurrentTaskContext();
  LOG_TRACE() << "WaitForEvent()";

  impl::WaitListLight::Lock lock;

  while (!signaled_.exchange(false)) {
    LOG_TRACE() << "iteration()";

    impl::TaskContext::SleepParams sleep_params;

    sleep_params.wait_list = lock_waiters_;
    sleep_params.exec_after_asleep = [this, &lock, current] {
      LOG_TRACE() << "exec_after_asleep()";
      lock_waiters_->Append(lock, current);
      if (signaled_)
        current->Wakeup(impl::TaskContext::WakeupSource::kWaitList);
    };
    sleep_params.exec_before_awake = [current, this]() {
      lock_waiters_->Remove(*current);
    };

    current->Sleep(std::move(sleep_params));
  }
  LOG_TRACE() << "exit";
}

void SingleConsumerEvent::Send() {
  signaled_ = true;

  impl::WaitListLight::Lock lock;
  lock_waiters_->WakeupOne(lock);
}

}  // namespace engine
