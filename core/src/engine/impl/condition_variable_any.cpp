#include <userver/engine/impl/condition_variable_any.hpp>

#include <userver/engine/mutex.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/utils/assert.hpp>

#include <engine/impl/wait_list.hpp>
#include <engine/task/task_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

template <typename MutexType>
class CvWaitStrategy final : public WaitStrategy {
 public:
  CvWaitStrategy(Deadline deadline, WaitList& waiters, TaskContext& current,
                 std::unique_lock<MutexType>& mutex_lock)
      : WaitStrategy(deadline),
        waiters_(waiters),
        waiter_token_(waiters_),
        waiters_lock_(waiters_),
        current_(current),
        mutex_lock_(mutex_lock) {}

  void SetupWakeups() override {
    UASSERT(waiters_lock_);
    UASSERT(&current_ == current_task::GetCurrentTaskContext());
    waiters_.Append(waiters_lock_, &current_);
    waiters_lock_.unlock();
    mutex_lock_.unlock();
  }

  void DisableWakeups() override {
    UASSERT(&current_ == current_task::GetCurrentTaskContext());
    waiters_lock_.lock();
    waiters_.Remove(waiters_lock_, &current_);
  }

 private:
  WaitList& waiters_;
  const WaitList::WaitersScopeCounter waiter_token_;
  WaitList::Lock waiters_lock_;
  TaskContext& current_;
  std::unique_lock<MutexType>& mutex_lock_;
};

template <typename MutexType>
ConditionVariableAny<MutexType>::ConditionVariableAny() : waiters_() {}

template <typename MutexType>
ConditionVariableAny<MutexType>::~ConditionVariableAny() = default;

template <typename MutexType>
CvStatus ConditionVariableAny<MutexType>::WaitUntil(
    std::unique_lock<MutexType>& lock, Deadline deadline) {
  if (deadline.IsReached()) {
    return CvStatus::kTimeout;
  }

  auto current = current_task::GetCurrentTaskContext();
  if (current->ShouldCancel()) {
    return CvStatus::kCancelled;
  }

  auto wakeup_source = TaskContext::WakeupSource::kNone;
  {
    CvWaitStrategy<MutexType> wait_manager(deadline, *waiters_, *current, lock);
    wakeup_source = current->Sleep(wait_manager);
  }
  // relock the mutex after it's been released in SetupWakeups()
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
}

template <typename MutexType>
void ConditionVariableAny<MutexType>::NotifyOne() {
  if (waiters_->GetCountOfSleepies()) {
    WaitList::Lock lock(*waiters_);
    waiters_->WakeupOne(lock);
  }
}

template <typename MutexType>
void ConditionVariableAny<MutexType>::NotifyAll() {
  if (waiters_->GetCountOfSleepies()) {
    WaitList::Lock lock(*waiters_);
    waiters_->WakeupAll(lock);
  }
}

template class ConditionVariableAny<std::mutex>;
template class ConditionVariableAny<Mutex>;

}  // namespace engine::impl

USERVER_NAMESPACE_END
