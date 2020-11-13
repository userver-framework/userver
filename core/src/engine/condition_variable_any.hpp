#pragma once

#include <memory>

#include <engine/condition_variable.hpp>
#include <engine/deadline.hpp>
#include <engine/task/cancel.hpp>
#include <utils/assert.hpp>

#include "task/task_context.hpp"
#include "wait_list.hpp"

namespace engine {
namespace impl {

template <typename MutexType>
class CvWaitStrategy final : public WaitStrategy {
 public:
  CvWaitStrategy(Deadline deadline, WaitList& waiters, TaskContext& current,
                 std::unique_lock<MutexType>& mutex_lock)
      : WaitStrategy(deadline),
        waiters_(waiters),
        waiter_token_(waiters_),
        lock_(waiters_),
        current_(current),
        mutex_lock_(mutex_lock) {}

  void AfterAsleep() override {
    UASSERT(&current_ == current_task::GetCurrentTaskContext());
    waiters_.Append(lock_, &current_);
    lock_.Release();
    mutex_lock_.unlock();
  }

  void BeforeAwake() override {
    UASSERT(&current_ == current_task::GetCurrentTaskContext());
    mutex_lock_.lock();
  }

  WaitListBase* GetWaitList() override { return &waiters_; }

 private:
  WaitList& waiters_;
  const WaitList::WaitersScopeCounter waiter_token_;
  WaitList::Lock lock_;
  TaskContext& current_;
  std::unique_lock<MutexType>& mutex_lock_;
};

template <typename MutexType>
class ConditionVariableAny {
 public:
  ConditionVariableAny() = default;

  ConditionVariableAny(const ConditionVariableAny&) = delete;
  ConditionVariableAny(ConditionVariableAny&&) = delete;
  ConditionVariableAny& operator=(const ConditionVariableAny&) = delete;
  ConditionVariableAny& operator=(ConditionVariableAny&&) = delete;

  [[nodiscard]] CvStatus Wait(std::unique_lock<MutexType>&);

  template <typename Predicate>
  [[nodiscard]] bool Wait(std::unique_lock<MutexType>&, Predicate&&);

  CvStatus WaitUntil(std::unique_lock<MutexType>&, Deadline);

  template <typename Predicate>
  bool WaitUntil(std::unique_lock<MutexType>&, Deadline, Predicate&&);

  void NotifyOne();
  void NotifyAll();

 private:
  WaitList waiters_;
};

template <typename MutexType>
CvStatus ConditionVariableAny<MutexType>::Wait(
    std::unique_lock<MutexType>& lock) {
  return WaitUntil(lock, {});
}

template <typename MutexType>
template <typename Predicate>
bool ConditionVariableAny<MutexType>::Wait(std::unique_lock<MutexType>& lock,
                                           Predicate&& predicate) {
  return WaitUntil(lock, {}, std::forward<Predicate>(predicate));
}

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

  CvWaitStrategy<MutexType> wait_manager(deadline, waiters_, *current, lock);
  current->Sleep(&wait_manager);

  switch (current->GetWakeupSource()) {
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
template <typename Predicate>
bool ConditionVariableAny<MutexType>::WaitUntil(
    std::unique_lock<MutexType>& lock, Deadline deadline,
    Predicate&& predicate) {
  bool predicate_result = predicate();
  auto status = CvStatus::kNoTimeout;
  while (!predicate_result && status == CvStatus::kNoTimeout) {
    status = WaitUntil(lock, deadline);
    predicate_result = predicate();
    if (!predicate_result && status == CvStatus::kNoTimeout) {
      current_task::AccountSpuriousWakeup();
    }
  }
  return predicate_result;
}

template <typename MutexType>
void ConditionVariableAny<MutexType>::NotifyOne() {
  if (waiters_.GetCountOfSleepies()) {
    WaitList::Lock lock(waiters_);
    waiters_.WakeupOne(lock);
  }
}

template <typename MutexType>
void ConditionVariableAny<MutexType>::NotifyAll() {
  if (waiters_.GetCountOfSleepies()) {
    WaitList::Lock lock(waiters_);
    waiters_.WakeupAll(lock);
  }
}

}  // namespace impl
}  // namespace engine
