#include <userver/engine/condition_variable.hpp>

#include "condition_variable_any.hpp"

namespace engine {

class ConditionVariable::Impl final : public impl::ConditionVariableAny<Mutex> {
};

ConditionVariable::ConditionVariable() : impl_(std::make_unique<Impl>()) {}

ConditionVariable::~ConditionVariable() = default;
ConditionVariable::ConditionVariable(ConditionVariable&&) noexcept = default;
ConditionVariable& ConditionVariable::operator=(ConditionVariable&&) noexcept =
    default;

void ConditionVariable::NotifyOne() { impl_->NotifyOne(); }

void ConditionVariable::NotifyAll() { impl_->NotifyAll(); }

CvStatus ConditionVariable::Wait(std::unique_lock<Mutex>& lock) {
  return impl_->Wait(lock);
}

CvStatus ConditionVariable::WaitUntil(std::unique_lock<Mutex>& lock,
                                      Deadline deadline) {
  return impl_->WaitUntil(lock, deadline);
}

}  // namespace engine
