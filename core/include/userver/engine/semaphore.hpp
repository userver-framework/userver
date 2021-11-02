#pragma once

/// @file userver/engine/semaphore.hpp
/// @brief @copybrief engine::Semaphore

#include <atomic>
#include <chrono>
#include <memory>
#include <shared_mutex>  // for std locks

#include <userver/engine/deadline.hpp>
#include <userver/engine/impl/wait_list_fwd.hpp>
#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

/// @ingroup userver_concurrency
///
/// @brief Class that allows up to `max_simultaneous_locks` concurrent accesses
/// to the critical section.
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
  /// @param max_simultaneous_locks initial number of available locks
  explicit Semaphore(Counter max_simultaneous_locks);

  ~Semaphore();

  Semaphore(Semaphore&&) = delete;
  Semaphore(const Semaphore&) = delete;
  Semaphore& operator=(Semaphore&&) = delete;
  Semaphore& operator=(const Semaphore&) = delete;

  /// Decrements internal semaphore lock counter. Blocks if current counter is
  /// zero until the subsequent call to unlock_shared() by another coroutine.
  /// @note the user must eventually call unlock_shared() to increment the lock
  /// counter.
  /// @note the method waits for the semaphore even if the current task is
  /// cancelled.
  void lock_shared();

  /// Increments internal semaphore lock counter. If there is a user waiting in
  /// lock_shared() on the same semaphore, it will be waken up.
  /// @note the order of coroutines to unblock is unspecified. Any code assuming
  /// any specific order (e.g. FIFO) is incorrent and must be fixed.
  /// @note it is allowed to call lock_shared() in one coroutine and
  /// subsequently call unlock_shared() in another coroutine. In particular, it
  /// is allowed to pass std::shared_lock<engine::Semaphore> across coroutines.
  void unlock_shared();

  bool try_lock_shared();

  template <typename Rep, typename Period>
  bool try_lock_shared_for(const std::chrono::duration<Rep, Period>&);

  template <typename Clock, typename Duration>
  bool try_lock_shared_until(const std::chrono::time_point<Clock, Duration>&);

  bool try_lock_shared_until(Deadline deadline);

  /// Returns an approximate number of available locks, use only for statistics.
  size_t RemainingApprox() const;

  void unlock_shared_count(Counter count);

  bool try_lock_shared_count(Counter count);

  bool try_lock_shared_until_count(Deadline deadline, Counter count);

 private:
  bool LockFastPath(Counter count);
  bool LockSlowPath(Deadline, Counter count);

  impl::FastPimplWaitList lock_waiters_;
  std::atomic<Counter> remaining_simultaneous_locks_;
  [[maybe_unused]] const Counter max_simultaneous_locks_;
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
  SemaphoreLock(Semaphore&, const std::chrono::duration<Rep, Period>&);

  template <typename Clock, typename Duration>
  SemaphoreLock(Semaphore&, const std::chrono::time_point<Clock, Duration>&);

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
  bool TryLockFor(const std::chrono::duration<Rep, Period>&);

  template <typename Clock, typename Duration>
  bool TryLockUntil(const std::chrono::time_point<Clock, Duration>&);

  bool TryLockUntil(Deadline);

  void Unlock();
  void Release();

 private:
  Semaphore* sem_{nullptr};
  bool owns_lock_{false};
};

template <typename Rep, typename Period>
bool Semaphore::try_lock_shared_for(
    const std::chrono::duration<Rep, Period>& duration) {
  return try_lock_shared_until(Deadline::FromDuration(duration));
}

template <typename Clock, typename Duration>
bool Semaphore::try_lock_shared_until(
    const std::chrono::time_point<Clock, Duration>& until) {
  return try_lock_shared_until(Deadline::FromTimePoint(until));
}

inline SemaphoreLock::SemaphoreLock(Semaphore& sem) : sem_(&sem) {
  sem_->lock_shared();
  owns_lock_ = true;
}

inline SemaphoreLock::SemaphoreLock(Semaphore& sem, std::defer_lock_t) noexcept
    : sem_(&sem) {}

inline SemaphoreLock::SemaphoreLock(Semaphore& sem, std::try_to_lock_t)
    : sem_(&sem) {
  TryLock();
}

inline SemaphoreLock::SemaphoreLock(Semaphore& sem, std::adopt_lock_t) noexcept
    : sem_(&sem), owns_lock_(true) {}

template <typename Rep, typename Period>
SemaphoreLock::SemaphoreLock(Semaphore& sem,
                             const std::chrono::duration<Rep, Period>& duration)
    : sem_(&sem) {
  TryLockFor(duration);
}

template <typename Clock, typename Duration>
SemaphoreLock::SemaphoreLock(
    Semaphore& sem, const std::chrono::time_point<Clock, Duration>& until)
    : sem_(&sem) {
  TryLockUntil(until);
}

inline SemaphoreLock::SemaphoreLock(Semaphore& sem, Deadline deadline)
    : sem_(&sem) {
  TryLockUntil(deadline);
}

inline SemaphoreLock& SemaphoreLock::operator=(SemaphoreLock&& other) noexcept {
  if (OwnsLock()) Unlock();
  sem_ = other.sem_;
  owns_lock_ = other.owns_lock_;
  other.owns_lock_ = false;

  return *this;
}

inline SemaphoreLock::SemaphoreLock(SemaphoreLock&& other) noexcept
    : sem_(other.sem_), owns_lock_(std::exchange(other.owns_lock_, false)) {}

inline SemaphoreLock::~SemaphoreLock() {
  if (OwnsLock()) Unlock();
}

inline bool SemaphoreLock::OwnsLock() const noexcept { return owns_lock_; }

inline void SemaphoreLock::Lock() {
  UASSERT(sem_);
  UASSERT(!owns_lock_);
  sem_->lock_shared();
}

inline bool SemaphoreLock::TryLock() {
  UASSERT(sem_);
  UASSERT(!owns_lock_);
  owns_lock_ = sem_->try_lock_shared();
  return owns_lock_;
}

template <typename Rep, typename Period>
bool SemaphoreLock::TryLockFor(
    const std::chrono::duration<Rep, Period>& duration) {
  UASSERT(sem_);
  UASSERT(!owns_lock_);
  owns_lock_ = sem_->try_lock_shared_for(duration);
  return owns_lock_;
}

template <typename Clock, typename Duration>
bool SemaphoreLock::TryLockUntil(
    const std::chrono::time_point<Clock, Duration>& until) {
  UASSERT(sem_);
  UASSERT(!owns_lock_);
  owns_lock_ = sem_->try_lock_shared_until(until);
  return owns_lock_;
}

inline bool SemaphoreLock::TryLockUntil(Deadline deadline) {
  UASSERT(sem_);
  UASSERT(!owns_lock_);
  owns_lock_ = sem_->try_lock_shared_until(deadline);
  return owns_lock_;
}

inline void SemaphoreLock::Unlock() {
  UASSERT(sem_);
  UASSERT(owns_lock_);
  sem_->unlock_shared();
  owns_lock_ = false;
}

inline void SemaphoreLock::Release() {
  sem_ = nullptr;
  owns_lock_ = false;
}

}  // namespace engine

USERVER_NAMESPACE_END
