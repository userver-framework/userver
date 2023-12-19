#pragma once

#include <boost/smart_ptr/intrusive_ptr.hpp>

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

  /// @brief Append the task to the `WaitListLight`
  /// @note To account for `WakeupOne()` calls between condition check and
  /// `Sleep` + `Append`, you have to recheck the condition after `Append`
  /// returns in `SetupWakeups`.
  /// @note Must not be used together with `SetSignalAndWakeupOne`.
  void Append(boost::intrusive_ptr<impl::TaskContext>&& context) noexcept;

  /// @brief Get the signal if one was set by SetSignalAndWakeupOne, else
  /// Append.
  /// @returns `true` if already signaled
  /// @see Append
  [[nodiscard]] bool GetSignalOrAppend(
      boost::intrusive_ptr<impl::TaskContext>&& context) noexcept;

  /// @brief Remove the task from the `WaitListLight` without wakeup.
  void Remove(impl::TaskContext& context) noexcept;

  /// @brief Wakes up the waiting task; the next waiter may not `Append` until
  /// `Remove` is called.
  void WakeupOne();

  /// @brief Sets signal, which will wake up future waiters. Wakes up the
  /// existing waiter, if any. The next waiter may not `Append` until
  /// `Remove` is called.
  /// @see GetSignalOrAppend
  void SetSignalAndWakeupOne();

  /// @brief Resets the notification, if any.
  /// @warning Reset with an active waiter is not allowed! A good rule of thumb
  /// is to only call this from the waiting task.
  bool GetAndResetSignal() noexcept;

  /// @returns Whether the signal was set and not yet reset
  bool IsSignaled() const noexcept;

 private:
  bool IsEmptyRelaxed() noexcept;

  struct Impl;
  utils::FastPimpl<Impl, 16, 16> impl_;
};

}  // namespace engine::impl

USERVER_NAMESPACE_END
