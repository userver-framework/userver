#include <engine/condition_variable.hpp>

#include "condition_variable_any.hpp"

namespace engine {

class ConditionVariable::Impl : public impl::ConditionVariableAny<Mutex> {};

ConditionVariable::ConditionVariable() : impl_(std::make_unique<Impl>()) {}

ConditionVariable::~ConditionVariable() = default;
ConditionVariable::ConditionVariable(ConditionVariable&&) noexcept = default;
ConditionVariable& ConditionVariable::operator=(ConditionVariable&&) noexcept =
    default;

void ConditionVariable::NotifyOne() { impl_->NotifyOne(); }

void ConditionVariable::NotifyAll() { impl_->NotifyAll(); }

void ConditionVariable::DoWait(std::unique_lock<Mutex>& lock) {
  impl_->Wait(lock);
}

std::cv_status ConditionVariable::DoWaitUntil(std::unique_lock<Mutex>& lock,
                                              Deadline deadline) {
  return impl_->WaitUntil(lock, std::move(deadline));
}

}  // namespace engine
