#pragma once

#include <atomic>

#include <userver/engine/mutex.hpp>
#include <userver/engine/semaphore.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

class SemaphoreCapacityControl final {
 public:
  using Counter = engine::Semaphore::Counter;

  explicit SemaphoreCapacityControl(engine::Semaphore& semaphore);

  void SetCapacity(Counter capacity);
  Counter GetCapacity() const noexcept;

  void SetCapacityOverride(Counter capacity);
  void RemoveCapacityOverride();

 private:
  struct CapacityData {
    Counter base;
    Counter override;
    bool override_enabled;
  };

  engine::Semaphore& semaphore_;
  engine::Mutex capacity_mutex_;
  std::atomic<Counter> capacity_;
  bool capacity_override_enabled_;
};

}  // namespace concurrent::impl

USERVER_NAMESPACE_END
