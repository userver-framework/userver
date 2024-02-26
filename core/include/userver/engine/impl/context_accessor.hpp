#pragma once

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class TaskContext;

template <typename T>
class FutureWaitStrategy;

enum class [[nodiscard]] EarlyWakeup : bool {};

class ContextAccessor {
 public:
  virtual bool IsReady() const noexcept = 0;

  // Atomically:
  // 1. if not `IsReady`, then store `waiter` somewhere to notify when
  //    `IsReady() == true` is reached, and return `EarlyWakeup{false}`;
  // 2. if `IsReady`, then notify `waiter` immediately via `Wakeup*`,
  //    and return `EarlyWakeup{true}`.
  virtual EarlyWakeup TryAppendWaiter(TaskContext& waiter) = 0;

  // Remove `waiter` from the internal wait list if it's still there.
  virtual void RemoveWaiter(TaskContext& waiter) noexcept = 0;

  // Precondition: IsReady
  // This method is required for WaitAllChecked to properly function.
  virtual void RethrowErrorResult() const = 0;

 protected:
  ContextAccessor();

  ~ContextAccessor() = default;
};

}  // namespace engine::impl

USERVER_NAMESPACE_END
