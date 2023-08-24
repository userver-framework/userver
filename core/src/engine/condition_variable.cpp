#include <userver/engine/condition_variable.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

ConditionVariable::ConditionVariable() = default;

ConditionVariable::~ConditionVariable() = default;

CvStatus ConditionVariable::Wait(std::unique_lock<Mutex>& lock) {
  return WaitUntil(lock, {});
}

CvStatus ConditionVariable::WaitUntil(std::unique_lock<Mutex>& lock,
                                      Deadline deadline) {
  return impl_.WaitUntil(lock, deadline);
}

void ConditionVariable::NotifyOne() { impl_.NotifyOne(); }

void ConditionVariable::NotifyAll() { impl_.NotifyAll(); }

}  // namespace engine

USERVER_NAMESPACE_END
