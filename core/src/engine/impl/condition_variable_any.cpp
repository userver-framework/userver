#include <userver/engine/impl/condition_variable_any.hpp>

#include <userver/engine/mutex.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/utils/assert.hpp>

#include <engine/impl/wait_list.hpp>
#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

void OnConditionVariableSpuriousWakeup() {
  current_task::AccountSpuriousWakeup();
}

template <typename MutexType>
class CvWaitStrategy final : public WaitStrategy {
 public:
  CvWaitStrategy(Deadline deadline, WaitList& waiters, TaskContext& current,
                 std::unique_lock<MutexType>& mutex_lock) noexcept
      : WaitStrategy(deadline),
        mutex_lock_(mutex_lock),
        waiter_scope_(waiters, current) {}

  void SetupWakeups() override {
    waiter_scope_.Append();
    mutex_lock_.unlock();
  }

  void DisableWakeups() override { waiter_scope_.Remove(); }

 private:
  std::unique_lock<MutexType>& mutex_lock_;
  WaitScope waiter_scope_;
};

template <typename MutexType>
ConditionVariableAny<MutexType>::ConditionVariableAny() = default;

template <typename MutexType>
ConditionVariableAny<MutexType>::~ConditionVariableAny() = default;

template <typename MutexType>
CvStatus ConditionVariableAny<MutexType>::WaitUntil(
    std::unique_lock<MutexType>& lock, Deadline deadline) {
  if (deadline.IsReached()) {
    return CvStatus::kTimeout;
  }

  auto& current = current_task::GetCurrentTaskContext();
  if (current.ShouldCancel()) {
    return CvStatus::kCancelled;
  }

  auto wakeup_source = TaskContext::WakeupSource::kNone;
  {
    CvWaitStrategy<MutexType> wait_manager(deadline, *waiters_, current, lock);
    wakeup_source = current.Sleep(wait_manager);
  }
  // re-lock the mutex after it's been released in SetupWakeups()
  lock.lock();

  switch (wakeup_source) {
    case TaskContext::WakeupSource::kCancelRequest:
      return CvStatus::kCancelled;
    case TaskContext::WakeupSource::kDeadlineTimer:
      return CvStatus::kTimeout;
    case TaskContext::WakeupSource::kNone:
    case TaskContext::WakeupSource::kBootstrap:
      UASSERT(!"invalid wakeup source");
      [[fallthrough]];
    case TaskContext::WakeupSource::kWaitList:
      return CvStatus::kNoTimeout;
  }

  UINVARIANT(false, "Unexpected wakeup source in ConditionVariableAny");
}

template <typename MutexType>
void ConditionVariableAny<MutexType>::NotifyOne() {
  waiters_->WakeupOne();
}

template <typename MutexType>
void ConditionVariableAny<MutexType>::NotifyAll() {
  waiters_->WakeupAll();
}

template class ConditionVariableAny<std::mutex>;
template class ConditionVariableAny<Mutex>;

}  // namespace engine::impl

USERVER_NAMESPACE_END
