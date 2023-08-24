#include <userver/concurrent/impl/semaphore_capacity_control.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

SemaphoreCapacityControl::SemaphoreCapacityControl(
    engine::CancellableSemaphore& semaphore)
    : semaphore_(semaphore), capacity_requested_(semaphore.GetCapacity()) {}

void SemaphoreCapacityControl::SetCapacity(Counter capacity) {
  capacity_requested_.store(capacity);
  UpdateSemaphoreCapacity();
}

auto SemaphoreCapacityControl::GetCapacity() const noexcept -> Counter {
  return capacity_requested_.load();
}

void SemaphoreCapacityControl::SetCapacityOverride(Counter capacity) {
  UASSERT(capacity != kOverrideDisabled);
  capacity_override_.store(capacity);
  UpdateSemaphoreCapacity();
}

void SemaphoreCapacityControl::RemoveCapacityOverride() {
  capacity_override_.store(kOverrideDisabled);
  UpdateSemaphoreCapacity();
}

void SemaphoreCapacityControl::UpdateSemaphoreCapacity() const {
  // This method synchronizes the logical Semaphore state contained
  // in capacity_*_ atomics with the actual Semaphore state.
  auto capacity_base = capacity_requested_.load();
  auto capacity_override = capacity_override_.load();
  auto capacity = capacity_override == kOverrideDisabled ? capacity_base
                                                         : capacity_override;

  // The loop is needed, because otherwise two concurrent
  // UpdateSemaphoreCapacity calls, executed in A-B-B-A order, will result in a
  // mismatch between our atomics and the actual Semaphore capacity.
  while (true) {
    semaphore_.SetCapacity(capacity);

    capacity_base = capacity_requested_.load();
    capacity_override = capacity_override_.load();
    const auto new_capacity = capacity_override == kOverrideDisabled
                                  ? capacity_base
                                  : capacity_override;
    if (new_capacity == capacity) break;

    capacity = new_capacity;
  }
}

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
