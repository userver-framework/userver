#include <userver/concurrent/impl/semaphore_capacity_control.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

SemaphoreCapacityControl::SemaphoreCapacityControl(engine::Semaphore& semaphore)
    : semaphore_(semaphore),
      capacity_(semaphore.GetCapacity()),
      capacity_override_enabled_(false) {}

void SemaphoreCapacityControl::SetCapacity(Counter capacity) {
  std::lock_guard lock(capacity_mutex_);
  capacity_.store(capacity);
  if (!capacity_override_enabled_) {
    semaphore_.SetCapacity(capacity);
  }
}

SemaphoreCapacityControl::Counter SemaphoreCapacityControl::GetCapacity() const
    noexcept {
  return capacity_.load();
}

void SemaphoreCapacityControl::SetCapacityOverride(Counter capacity) {
  std::lock_guard lock(capacity_mutex_);
  capacity_override_enabled_ = true;
  semaphore_.SetCapacity(capacity);
}

void SemaphoreCapacityControl::RemoveCapacityOverride() {
  std::lock_guard lock(capacity_mutex_);
  capacity_override_enabled_ = false;
  semaphore_.SetCapacity(capacity_.load());
}

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
