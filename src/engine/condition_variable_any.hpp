#pragma once

#include <condition_variable>
#include <memory>

#include <engine/deadline.hpp>

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

  void Wait(std::unique_lock<MutexType>&);

  template <typename Predicate>
  void Wait(std::unique_lock<MutexType>&, Predicate);

  std::cv_status WaitUntil(std::unique_lock<MutexType>&, Deadline);

  template <typename Predicate>
  bool WaitUntil(std::unique_lock<MutexType>&, Deadline, Predicate);

  void NotifyOne();
  void NotifyAll();

 private:
  std::shared_ptr<WaitList> waiters_;
};

template <typename MutexType>
ConditionVariableAny<MutexType>::ConditionVariableAny()
    : waiters_(std::make_shared<WaitList>()) {}

template <typename MutexType>
void ConditionVariableAny<MutexType>::Wait(std::unique_lock<MutexType>& lock) {
  WaitUntil(lock, {});
}

template <typename MutexType>
template <typename Predicate>
void ConditionVariableAny<MutexType>::Wait(std::unique_lock<MutexType>& lock,
                                           Predicate predicate) {
  while (!predicate()) {
    Wait(lock);
  }
}

template <typename MutexType>
std::cv_status ConditionVariableAny<MutexType>::WaitUntil(
    std::unique_lock<MutexType>& lock, Deadline deadline) {
  if (deadline != Deadline{} && deadline < Deadline::clock::now()) {
    return std::cv_status::timeout;
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

  return context->GetWakeupSource() ==
                 impl::TaskContext::WakeupSource::kDeadlineTimer
             ? std::cv_status::timeout
             : std::cv_status::no_timeout;
}

template <typename MutexType>
template <typename Predicate>
bool ConditionVariableAny<MutexType>::WaitUntil(
    std::unique_lock<MutexType>& lock, Deadline deadline, Predicate predicate) {
  bool predicate_result = predicate();
  auto status = std::cv_status::no_timeout;
  while (!predicate_result && status == std::cv_status::no_timeout) {
    status = WaitUntil(lock, std::move(deadline));
    predicate_result = predicate();
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
