#pragma once

#include <atomic>
#include <chrono>
#include <memory>
#include <shared_mutex>  // for std locks

#include <engine/deadline.hpp>
#include <utils/assert.hpp>

namespace engine {

namespace impl {
class WaitList;
}  // namespace impl

/// Class that allows `max_simultaneous_locks` concurrent accesses to the
/// critical section.
///
/// Example:
///   engine::Semaphore sem{2};
///   std::unique_lock<engine::Semaphore> lock{sem};
///   utils::Async("work", [&sem]() {
///       std::lock_guard<engine::Semaphore> guard{sem};
///       // ...
///   }).Detach();
class Semaphore final {
 public:
  /// Creates a semaphore with predefined number of available locks
  /// @param max_simultaneous_locks initial number of available locks
  explicit Semaphore(std::size_t max_simultaneous_locks);
  ~Semaphore() = default;

  Semaphore(Semaphore&&) = delete;
  Semaphore(const Semaphore&) = delete;
  Semaphore& operator=(Semaphore&&) = delete;
  Semaphore& operator=(const Semaphore&) = delete;

  void lock_shared();
  void unlock_shared();
  bool try_lock_shared();

  template <typename Rep, typename Period>
  bool try_lock_shared_for(const std::chrono::duration<Rep, Period>&);

  template <typename Clock, typename Duration>
  bool try_lock_shared_until(const std::chrono::time_point<Clock, Duration>&);

  bool try_lock_shared_until(Deadline deadline);

  /// Returns an approximate number of available locks, use only for statistics.
  size_t RemainingApprox() const;

 private:
  bool LockFastPath();
  bool LockSlowPath(Deadline);

  std::shared_ptr<impl::WaitList> lock_waiters_;
  std::atomic<std::size_t> remaining_simultaneous_locks_;
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
  SemaphoreLock(SemaphoreLock&&) noexcept = default;
  SemaphoreLock& operator=(const SemaphoreLock&) = delete;
  SemaphoreLock& operator=(SemaphoreLock&&) noexcept = default;

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
