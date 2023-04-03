#pragma once

/// @file userver/engine/semaphore.hpp
/// @brief @copybrief engine::Semaphore

#include <atomic>
#include <chrono>
#include <shared_mutex>  // for std locks
#include <stdexcept>

#include <userver/engine/deadline.hpp>
#include <userver/engine/impl/wait_list_fwd.hpp>

// TODO remove extra includes
#include <memory>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

/// Thrown by engine::Semaphore when an amount of locks greater than its current
/// capacity is requested.
class UnreachableSemaphoreLockError final : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

/// Thrown by engine::CancellableSemaphore on current task cancellation
class SemaphoreLockCancelledError final : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

/// @ingroup userver_concurrency
///
/// @brief Class that allows up to `max_simultaneous_locks` concurrent accesses
/// to the critical section. It honours task cancellation, unlike Semaphore.
///
/// ## Example usage:
///
/// @snippet engine/semaphore_test.cpp  Sample engine::Semaphore usage
///
/// @see @ref md_en_userver_synchronization
class CancellableSemaphore final {
 public:
  using Counter = std::size_t;

  /// Creates a semaphore with predefined number of available locks
  /// @param capacity initial number of available locks
  explicit CancellableSemaphore(Counter capacity);

  ~CancellableSemaphore();

  CancellableSemaphore(CancellableSemaphore&&) = delete;
  CancellableSemaphore(const CancellableSemaphore&) = delete;
  CancellableSemaphore& operator=(CancellableSemaphore&&) = delete;
  CancellableSemaphore& operator=(const CancellableSemaphore&) = delete;

  /// Sets the total number of available locks. If the lock count decreases, the
  /// current acquired lock count may temporarily go above the limit.
  void SetCapacity(Counter capacity);

  /// Gets the total number of available locks.
  Counter GetCapacity() const noexcept;

  /// Returns an approximate number of available locks, use only for statistics.
  std::size_t RemainingApprox() const;

  /// Returns an approximate number of used locks, use only for statistics.
  std::size_t UsedApprox() const;

  /// Decrements internal semaphore lock counter. Blocks if current counter is
  /// zero until the subsequent call to unlock_shared() by another coroutine.
  /// @note the user must eventually call unlock_shared() to increment the lock
  /// counter.
  /// @note the method doesn't wait for the semaphore if the current task is
  /// cancelled. If a task waits on CancellableSemaphore and the cancellation
  /// is requested, the waiting is aborted with an exception.
  /// @throws UnreachableSemaphoreLockError if `capacity == 0`
  /// @throws SemaphoreLockCancelledError if the current task is cancelled
  void lock_shared();

  /// Increments internal semaphore lock counter. If there is a user waiting in
  /// lock_shared() on the same semaphore, it will be waken up.
  /// @note the order of coroutines to unblock is unspecified. Any code assuming
  /// any specific order (e.g. FIFO) is incorrect and must be fixed.
  /// @note it is allowed to call lock_shared() in one coroutine and
  /// subsequently call unlock_shared() in another coroutine. In particular, it
  /// is allowed to pass std::shared_lock<engine::Semaphore> across coroutines.
  void unlock_shared();

  [[nodiscard]] bool try_lock_shared();

  template <typename Rep, typename Period>
  [[nodiscard]] bool try_lock_shared_for(std::chrono::duration<Rep, Period>);

  template <typename Clock, typename Duration>
  [[nodiscard]] bool try_lock_shared_until(
      std::chrono::time_point<Clock, Duration>);

  [[nodiscard]] bool try_lock_shared_until(Deadline deadline);

  void lock_shared_count(Counter count);

  void unlock_shared_count(Counter count);

  [[nodiscard]] bool try_lock_shared_count(Counter count);

  [[nodiscard]] bool try_lock_shared_until_count(Deadline deadline,
                                                 Counter count);

 private:
  enum class TryLockStatus { kSuccess, kTransientFailure, kPermanentFailure };

  TryLockStatus DoTryLock(Counter count);
  TryLockStatus LockFastPath(Counter count);
  bool LockSlowPath(Deadline, Counter count);

  impl::FastPimplWaitList lock_waiters_;
  std::atomic<Counter> acquired_locks_;
  std::atomic<Counter> capacity_;
};

/// @ingroup userver_concurrency
///
/// @brief Class that allows up to `max_simultaneous_locks` concurrent accesses
/// to the critical section. It ignores task cancellation, unlike
/// CancellableSemaphore.
///
/// ## Example usage:
///
/// @snippet engine/semaphore_test.cpp  Sample engine::Semaphore usage
///
/// @see @ref md_en_userver_synchronization
class Semaphore final {
 public:
  using Counter = std::size_t;

  /// Creates a semaphore with predefined number of available locks
  /// @param capacity initial number of available locks
  explicit Semaphore(Counter capacity);

  ~Semaphore();

  Semaphore(Semaphore&&) = delete;
  Semaphore(const Semaphore&) = delete;
  Semaphore& operator=(Semaphore&&) = delete;
  Semaphore& operator=(const Semaphore&) = delete;

  /// Sets the total number of available locks. If the lock count decreases, the
  /// current acquired lock count may temporarily go above the limit.
  void SetCapacity(Counter capacity);

  /// Gets the total number of available locks.
  Counter GetCapacity() const noexcept;

  /// Returns an approximate number of available locks, use only for statistics.
  std::size_t RemainingApprox() const;

  /// Returns an approximate number of used locks, use only for statistics.
  std::size_t UsedApprox() const;

  /// Decrements internal semaphore lock counter. Blocks if current counter is
  /// zero until the subsequent call to unlock_shared() by another coroutine.
  /// @note the user must eventually call unlock_shared() to increment the lock
  /// counter.
  /// @note the method waits for the semaphore even if the current task is
  /// cancelled.
  /// @throws UnreachableSemaphoreLockError if `capacity == 0`
  void lock_shared();

  /// Increments internal semaphore lock counter. If there is a user waiting in
  /// lock_shared() on the same semaphore, it will be waken up.
  /// @note the order of coroutines to unblock is unspecified. Any code assuming
  /// any specific order (e.g. FIFO) is incorrect and must be fixed.
  /// @note it is allowed to call lock_shared() in one coroutine and
  /// subsequently call unlock_shared() in another coroutine. In particular, it
  /// is allowed to pass std::shared_lock<engine::Semaphore> across coroutines.
  void unlock_shared();

  [[nodiscard]] bool try_lock_shared();

  template <typename Rep, typename Period>
  [[nodiscard]] bool try_lock_shared_for(std::chrono::duration<Rep, Period>);

  template <typename Clock, typename Duration>
  [[nodiscard]] bool try_lock_shared_until(
      std::chrono::time_point<Clock, Duration>);

  [[nodiscard]] bool try_lock_shared_until(Deadline deadline);

  void lock_shared_count(Counter count);

  void unlock_shared_count(Counter count);

  [[nodiscard]] bool try_lock_shared_count(Counter count);

  [[nodiscard]] bool try_lock_shared_until_count(Deadline deadline,
                                                 Counter count);

 private:
  CancellableSemaphore sem_;
};

/// A replacement for std::shared_lock that accepts Deadline arguments
class SemaphoreLock final {
 public:
  SemaphoreLock() noexcept = default;
  explicit SemaphoreLock(Semaphore&);
  SemaphoreLock(Semaphore&, std::defer_lock_t) noexcept;
  SemaphoreLock(Semaphore&, std::try_to_lock_t);
  SemaphoreLock(Semaphore&, std::adopt_lock_t) noexcept;

  template <typename Rep, typename Period>
  SemaphoreLock(Semaphore&, std::chrono::duration<Rep, Period>);

  template <typename Clock, typename Duration>
  SemaphoreLock(Semaphore&, std::chrono::time_point<Clock, Duration>);

  SemaphoreLock(Semaphore&, Deadline);

  ~SemaphoreLock();

  SemaphoreLock(const SemaphoreLock&) = delete;
  SemaphoreLock(SemaphoreLock&&) noexcept;
  SemaphoreLock& operator=(const SemaphoreLock&) = delete;
  SemaphoreLock& operator=(SemaphoreLock&&) noexcept;

  bool OwnsLock() const noexcept;
  explicit operator bool() const noexcept { return OwnsLock(); }

  void Lock();
  bool TryLock();

  template <typename Rep, typename Period>
  bool TryLockFor(std::chrono::duration<Rep, Period>);

  template <typename Clock, typename Duration>
  bool TryLockUntil(std::chrono::time_point<Clock, Duration>);

  bool TryLockUntil(Deadline);

  void Unlock();
  void Release();

 private:
  Semaphore* sem_{nullptr};
  bool owns_lock_{false};
};

template <typename Rep, typename Period>
bool Semaphore::try_lock_shared_for(
    std::chrono::duration<Rep, Period> duration) {
  return try_lock_shared_until(Deadline::FromDuration(duration));
}

template <typename Clock, typename Duration>
bool Semaphore::try_lock_shared_until(
    std::chrono::time_point<Clock, Duration> until) {
  return try_lock_shared_until(Deadline::FromTimePoint(until));
}

template <typename Rep, typename Period>
bool CancellableSemaphore::try_lock_shared_for(
    std::chrono::duration<Rep, Period> duration) {
  return try_lock_shared_until(Deadline::FromDuration(duration));
}

template <typename Clock, typename Duration>
bool CancellableSemaphore::try_lock_shared_until(
    std::chrono::time_point<Clock, Duration> until) {
  return try_lock_shared_until(Deadline::FromTimePoint(until));
}

template <typename Rep, typename Period>
SemaphoreLock::SemaphoreLock(Semaphore& sem,
                             std::chrono::duration<Rep, Period> duration)
    : sem_(&sem) {
  TryLockFor(duration);
}

template <typename Clock, typename Duration>
SemaphoreLock::SemaphoreLock(Semaphore& sem,
                             std::chrono::time_point<Clock, Duration> until)
    : sem_(&sem) {
  TryLockUntil(until);
}

template <typename Rep, typename Period>
bool SemaphoreLock::TryLockFor(std::chrono::duration<Rep, Period> duration) {
  return TryLockUntil(Deadline::FromDuration(duration));
}

template <typename Clock, typename Duration>
bool SemaphoreLock::TryLockUntil(
    std::chrono::time_point<Clock, Duration> until) {
  return TryLockUntil(Deadline::FromTimePoint(until));
}

}  // namespace engine

USERVER_NAMESPACE_END
