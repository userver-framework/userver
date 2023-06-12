#pragma once

#include <atomic>

#include <userver/engine/semaphore.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

// Used by concurrent queues. Context:
// - engine::Semaphore controls queue size, e.g. a writer
//   takes 1 from the semaphore before pushing an element
// - queue size can be changed at any time (e.g. by a user dynamic_config hook)
// - the queue may need to "unblock" itself by overriding the capacity to +inf
// - while the capacity is temporarily overridden, a "normal" capacity
//   update may arrive
// - such an update must not be missed and must take effect after the queue
//   is "unblocked"
class SemaphoreCapacityControl final {
 public:
  using Counter = engine::Semaphore::Counter;
  static constexpr Counter kOverrideDisabled = -1;

  explicit SemaphoreCapacityControl(engine::CancellableSemaphore& semaphore);

  // These methods may be called concurrently from N threads.
  void SetCapacity(Counter capacity);
  Counter GetCapacity() const noexcept;

  // These methods may be called from 1 thread at a time, potentially
  // concurrently with *Capacity.
  void SetCapacityOverride(Counter capacity);
  void RemoveCapacityOverride();

 private:
  void UpdateSemaphoreCapacity() const;

  engine::CancellableSemaphore& semaphore_;
  std::atomic<Counter> capacity_requested_;
  std::atomic<Counter> capacity_override_{kOverrideDisabled};
};

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
