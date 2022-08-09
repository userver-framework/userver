#pragma once

#include <cstdint>

#include <boost/atomic/atomic.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class TaskContext;

struct alignas(sizeof(void*) * 2) Waiter final {
  TaskContext* context{nullptr};
  std::uintptr_t epoch{0};
};

class AtomicWaiter final {
 public:
  AtomicWaiter() noexcept;

  bool IsEmpty() noexcept;
  void Set(Waiter new_value) noexcept;
  Waiter GetAndReset() noexcept;

 private:
  boost::atomic<Waiter> waiter_;
};

}  // namespace engine::impl

USERVER_NAMESPACE_END
