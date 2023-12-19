#pragma once

/// @file userver/engine/shared_mutex.hpp
/// @brief @copybrief engine::SharedMutex

#include <userver/engine/condition_variable.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/semaphore.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

/// @ingroup userver_concurrency
///
/// @brief std::shared_mutex replacement for asynchronous tasks.
///
/// Ignores task cancellations (succeeds even if the current task is cancelled).
///
/// Writers (unique locks) have priority over readers (shared locks),
/// thus new shared lock waits for the pending writes to finish, which in turn
/// waits for existing existing shared locks to unlock first.
///
/// ## Example usage:
///
/// @snippet engine/shared_mutex_test.cpp  Sample engine::SharedMutex usage
///
/// @see @ref scripts/docs/en/userver/synchronization.md
class SharedMutex final {
 public:
  SharedMutex();
  ~SharedMutex() = default;

  SharedMutex(const SharedMutex&) = delete;
  SharedMutex(SharedMutex&&) = delete;
  SharedMutex& operator=(const SharedMutex&) = delete;
  SharedMutex& operator=(SharedMutex&&) = delete;

  /// Locks the mutex for unique ownership. Blocks current coroutine if the
  /// mutex is locked by another coroutine for reading or writing.
  ///
  /// @note The method waits for the mutex even if the current task is
  /// cancelled.
  void lock();

  /// Unlocks the mutex for unique ownership. Before calling this method the
  /// the mutex should be locked for unique ownership by current coroutine.
  ///
  /// @note the order of coroutines to unblock is unspecified. Any code assuming
  /// any specific order (e.g. FIFO) is incorrect and should be fixed.
  void unlock();

  /// Tries to lock the mutex for unique ownership without blocking the
  /// coroutine, returns true if succeeded.
  [[nodiscard]] bool try_lock();

  /// Tries to lock the mutex for unique ownership in specified duration.
  /// Blocks current coroutine if
  /// the mutex is locked by another coroutine up to the provided duration.
  ///
  /// @returns true if the locking succeeded
  template <typename Rep, typename Period>
  [[nodiscard]] bool try_lock_for(const std::chrono::duration<Rep, Period>&);

  /// Tries to lock the mutex for unique ownership till specified time point.
  /// Blocks current coroutine if
  /// the mutex is locked by another coroutine up to the provided duration.
  ///
  /// @returns true if the locking succeeded
  template <typename Clock, typename Duration>
  [[nodiscard]] bool try_lock_until(
      const std::chrono::time_point<Clock, Duration>&);

  /// @overload
  [[nodiscard]] bool try_lock_until(Deadline deadline);

  /// Locks the mutex for shared ownership. Blocks current coroutine if the
  /// mutex is locked by another coroutine for reading or writing.
  ///
  /// @note The method waits for the mutex even if the current task is
  /// cancelled.
  void lock_shared();

  /// Unlocks the mutex for shared ownership. Before calling this method the
  /// mutex should be locked for shared ownership by current coroutine.
  ///
  /// @note the order of coroutines to unblock is unspecified. Any code assuming
  /// any specific order (e.g. FIFO) is incorrect and should be fixed.
  void unlock_shared();

  /// Tries to lock the mutex for shared ownership without blocking the
  /// coroutine, returns true if succeeded.
  [[nodiscard]] bool try_lock_shared();

  /// Tries to lock the mutex for shared ownership in specified duration.
  /// Blocks current coroutine if
  /// the mutex is locked by another coroutine up to the provided duration.
  ///
  /// @returns true if the locking succeeded
  template <typename Rep, typename Period>
  [[nodiscard]] bool try_lock_shared_for(
      const std::chrono::duration<Rep, Period>&);

  /// Tries to lock the mutex for shared ownership till specified time point.
  /// Blocks current coroutine if
  /// the mutex is locked by another coroutine up to the provided duration.
  ///
  /// @returns true if the locking succeeded
  template <typename Clock, typename Duration>
  [[nodiscard]] bool try_lock_shared_until(
      const std::chrono::time_point<Clock, Duration>&);

  /// @overload
  [[nodiscard]] bool try_lock_shared_until(Deadline deadline);

 private:
  bool HasWaitingWriter() const noexcept;

  bool WaitForNoWaitingWriters(Deadline deadline);

  void DecWaitingWriters();

  /* Semaphore can be get by 1 or by SIZE_MAX.
   * 1 = reader, SIZE_MAX = writer.
   *
   * Three possible cases:
   * 1) semaphore is free
   * 2) there are readers in critical section (any count)
   * 3) there is a single writer in critical section
   */
  Semaphore semaphore_;

  /* Readers don't try to hold semaphore_ if there is at least one
   * waiting writer => writers don't starve.
   */
  std::atomic_size_t waiting_writers_count_;
  Mutex waiting_writers_count_mutex_;
  ConditionVariable waiting_writers_count_cv_;
};

template <typename Rep, typename Period>
bool SharedMutex::try_lock_for(
    const std::chrono::duration<Rep, Period>& duration) {
  return try_lock_until(Deadline::FromDuration(duration));
}

template <typename Rep, typename Period>
bool SharedMutex::try_lock_shared_for(
    const std::chrono::duration<Rep, Period>& duration) {
  return try_lock_shared_until(Deadline::FromDuration(duration));
}

template <typename Clock, typename Duration>
bool SharedMutex::try_lock_until(
    const std::chrono::time_point<Clock, Duration>& until) {
  return try_lock_until(Deadline::FromTimePoint(until));
}

template <typename Clock, typename Duration>
bool SharedMutex::try_lock_shared_until(
    const std::chrono::time_point<Clock, Duration>& until) {
  return try_lock_shared_until(Deadline::FromTimePoint(until));
}

}  // namespace engine

USERVER_NAMESPACE_END
