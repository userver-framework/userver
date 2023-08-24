#pragma once

#include <mutex>  // for std::unique_lock

#include <userver/engine/condition_variable_status.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/impl/wait_list_fwd.hpp>

// TODO remove extra includes
#include <userver/engine/task/task.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

void OnConditionVariableSpuriousWakeup();

template <typename MutexType>
class ConditionVariableAny {
 public:
  ConditionVariableAny();
  ~ConditionVariableAny();

  ConditionVariableAny(const ConditionVariableAny&) = delete;
  ConditionVariableAny(ConditionVariableAny&&) = delete;
  ConditionVariableAny& operator=(const ConditionVariableAny&) = delete;
  ConditionVariableAny& operator=(ConditionVariableAny&&) = delete;

  CvStatus WaitUntil(std::unique_lock<MutexType>&, Deadline);

  template <typename Predicate>
  bool WaitUntil(std::unique_lock<MutexType>&, Deadline, Predicate&&);

  void NotifyOne();
  void NotifyAll();

 private:
  FastPimplWaitList waiters_;
};

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
      impl::OnConditionVariableSpuriousWakeup();
    }
  }
  return predicate_result;
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
