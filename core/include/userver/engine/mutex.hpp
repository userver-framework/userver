#pragma once

/// @file userver/engine/mutex.hpp
/// @brief @copybrief engine::Mutex

#include <atomic>
#include <chrono>
#include <mutex>  // for std locks

#include <userver/engine/deadline.hpp>
#include <userver/engine/impl/wait_list_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

/// @ingroup userver_concurrency
///
/// @brief std::mutex replacement for asynchronous tasks.
///
/// Ignores task cancellations (succeeds even if the current task is cancelled).
///
/// ## Example usage:
///
/// @snippet engine/mutex_test.cpp  Sample engine::Mutex usage
///
/// @see @ref scripts/docs/en/userver/synchronization.md
class Mutex final {
 public:
  Mutex();
  ~Mutex();

  Mutex(const Mutex&) = delete;
  Mutex(Mutex&&) = delete;
  Mutex& operator=(const Mutex&) = delete;
  Mutex& operator=(Mutex&&) = delete;

  /// Locks the mutex. Blocks current coroutine if the mutex is locked by
  /// another coroutine. Throws if a coroutine tries to lock a mutex
  /// which is already locked by the current coroutine.
  ///
  /// @note The method waits for the mutex even if the current task is
  /// cancelled.
  void lock();

  /// Unlocks the mutex. Before calling this method the mutex should be locked
  /// by the current coroutine.
  ///
  /// @note the order of coroutines to unblock is unspecified. Any code assuming
  /// any specific order (e.g. FIFO) is incorrect and should be fixed.
  void unlock();

  /// Tries to lock the mutex without blocking the coroutine, returns true if
  /// succeeded.
  ///
  /// @note The behavior of the function is not affected by cancelation request.
  [[nodiscard]] bool try_lock() noexcept;

  /// Tries to lock the mutex in specified duration. Blocks current coroutine if
  /// the mutex is locked by another coroutine up to the provided duration.
  /// Throws if a coroutine tries to lock a mutex
  /// which is already locked by the current coroutine.
  ///
  /// @returns true if the locking succeeded
  ///
  /// @note The method waits for the mutex even if the current task is
  /// cancelled.
  template <typename Rep, typename Period>
  [[nodiscard]] bool try_lock_for(const std::chrono::duration<Rep, Period>&);

  /// Tries to lock the mutex till specified time point. Blocks current
  /// coroutine if the mutex is locked by another coroutine up to the provided
  /// time point. Throws if a coroutine tries to lock a mutex
  /// which is already locked by the current coroutine.
  ///
  /// @returns true if the locking succeeded
  ///
  /// @note The method waits for the mutex even if the current task is
  /// cancelled.
  template <typename Clock, typename Duration>
  [[nodiscard]] bool try_lock_until(
      const std::chrono::time_point<Clock, Duration>&);

  /// @overload
  [[nodiscard]] bool try_lock_until(Deadline deadline);

 private:
  class Impl;

  utils::FastPimpl<Impl, 96, alignof(void*)> impl_;
};

template <typename Rep, typename Period>
bool Mutex::try_lock_for(const std::chrono::duration<Rep, Period>& duration) {
  return try_lock_until(Deadline::FromDuration(duration));
}

template <typename Clock, typename Duration>
bool Mutex::try_lock_until(
    const std::chrono::time_point<Clock, Duration>& until) {
  return try_lock_until(Deadline::FromTimePoint(until));
}

}  // namespace engine

USERVER_NAMESPACE_END
