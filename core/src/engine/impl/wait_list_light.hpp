#pragma once

#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

class TaskContext;

/// Wait list for a single entry. All functions are thread-safe.
class WaitListLight final {
 public:
  /// Create an empty `WaitListLight`
  WaitListLight() noexcept;

  WaitListLight(const WaitListLight&) = delete;
  WaitListLight(WaitListLight&&) = delete;
  WaitListLight& operator=(const WaitListLight&) = delete;
  WaitListLight& operator=(WaitListLight&&) = delete;
  ~WaitListLight();

  /// @brief Wakes up the waiting task; the next waiter may not `Append` until
  /// `Remove` is called.
  /// @returns `true` if we caused the wakeup of a task.
  void WakeupOne();

 private:
  friend class WaitScopeLight;

  TaskContext* GetWaiterRelaxed() noexcept;

  struct Impl;
  utils::FastPimpl<Impl, 16, 16> impl_;
};

class WaitScopeLight final {
 public:
  explicit WaitScopeLight(WaitListLight& owner, TaskContext& context);

  WaitScopeLight(WaitScopeLight&&) = delete;
  WaitScopeLight&& operator=(WaitScopeLight&&) = delete;
  ~WaitScopeLight();

  TaskContext& GetContext() const noexcept { return context_; }

  /// @brief Append the task to the `WaitListLight`
  /// @note To account for `WakeupOne()` calls between condition check and
  /// `Sleep` + `Append`, you have to recheck the condition after `Append`
  /// returns in `SetupWakeups`.
  void Append() noexcept;

  /// @brief Remove the task from the `WaitListLight` without wakeup.
  /// @returns `true` if the task was still there and was removed by this call.
  void Remove() noexcept;

 private:
  WaitListLight& owner_;
  TaskContext& context_;
};

}  // namespace engine::impl

USERVER_NAMESPACE_END
