#pragma once

#include <memory>

#include <engine/condition_variable.hpp>
#include <engine/deadline.hpp>
#include <engine/task/cancel.hpp>

#include "task/task_context.hpp"
#include "wait_list.hpp"

namespace engine {
namespace impl {

template <typename MutexType>
class ConditionVariableAny {
 public:
  ConditionVariableAny();

  ConditionVariableAny(const ConditionVariableAny&) = delete;
  ConditionVariableAny(ConditionVariableAny&&) noexcept = default;
  ConditionVariableAny& operator=(const ConditionVariableAny&) = delete;
  ConditionVariableAny& operator=(ConditionVariableAny&&) noexcept = default;

  [[nodiscard]] CvStatus Wait(std::unique_lock<MutexType>&);

  template <typename Predicate>
  [[nodiscard]] bool Wait(std::unique_lock<MutexType>&, Predicate&&);

  CvStatus WaitUntil(std::unique_lock<MutexType>&, Deadline);

  template <typename Predicate>
  bool WaitUntil(std::unique_lock<MutexType>&, Deadline, Predicate&&);

  void NotifyOne();
  void NotifyAll();

 private:
  std::shared_ptr<WaitList> waiters_;
};

template <typename MutexType>
ConditionVariableAny<MutexType>::ConditionVariableAny()
    : waiters_(std::make_shared<WaitList>()) {}

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
  if (current_task::ShouldCancel()) {
    return CvStatus::kCancelled;
  }

  WaitList::Lock waiters_lock(*waiters_);

  auto context = current_task::GetCurrentTaskContext();

  impl::TaskContext::SleepParams new_sleep_params;
  new_sleep_params.deadline = std::move(deadline);
  new_sleep_params.wait_list = waiters_;
  new_sleep_params.exec_after_asleep = [this, &lock, &waiters_lock, context] {
    waiters_->Append(waiters_lock, context);
    waiters_lock.Release();
    lock.unlock();
  };
  new_sleep_params.exec_before_awake = [&lock] { lock.lock(); };
  context->Sleep(std::move(new_sleep_params));

  switch (context->GetWakeupSource()) {
    case TaskContext::WakeupSource::kCancelRequest:
      return CvStatus::kCancelled;
    case TaskContext::WakeupSource::kDeadlineTimer:
      return CvStatus::kTimeout;
    case TaskContext::WakeupSource::kNone:
      assert(!"invalid wakeup source");
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
    status = WaitUntil(lock, std::move(deadline));
    predicate_result = predicate();
    if (!predicate_result && status == CvStatus::kNoTimeout) {
      current_task::AccountSpuriousWakeup();
    }
  }
  return predicate_result;
}

template <typename MutexType>
void ConditionVariableAny<MutexType>::NotifyOne() {
  WaitList::Lock lock(*waiters_);
  waiters_->WakeupOne(lock);
}

template <typename MutexType>
void ConditionVariableAny<MutexType>::NotifyAll() {
  WaitList::Lock lock(*waiters_);
  waiters_->WakeupAll(lock);
}

}  // namespace impl
}  // namespace engine
