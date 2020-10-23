#pragma once

/// @file engine/mutex.hpp
/// @brief @copybrief engine::Mutex

#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>  // for std locks

#include <engine/deadline.hpp>
#include <engine/wait_list_fwd.hpp>

namespace engine {

/// std::mutex replacement for asynchronous tasks
class Mutex final {
 public:
  Mutex();
  ~Mutex();

  Mutex(const Mutex&) = delete;
  Mutex(Mutex&&) = delete;
  Mutex& operator=(const Mutex&) = delete;
  Mutex& operator=(Mutex&&) = delete;

  /// Locks the mutex. Blocks current coroutine if the mutex is locked by
  /// another coroutine.
  /// @note the behaviour is undefined if a coroutine tries to lock a mutex
  /// which is already locked by the current coroutine.
  /// @note the method waits for the mutex even if the current task is
  /// cancelled.
  void lock();

  /// Unlocks the mutex. The mutex must be locked by the current coroutine.
  /// @note the behaviour is undefined if a coroutine tries to unlock a mutex
  /// which is not locked or is locked by another coroutine
  /// @note the order of coroutines to unblock is unspecified. Any code assuming
  /// any specific order (e.g. FIFO) is incorrent and must be fixed.
  void unlock();

  bool try_lock();

  template <typename Rep, typename Period>
  bool try_lock_for(const std::chrono::duration<Rep, Period>&);

  template <typename Clock, typename Duration>
  bool try_lock_until(const std::chrono::time_point<Clock, Duration>&);

  bool try_lock_until(Deadline deadline);

 private:
  bool LockFastPath(impl::TaskContext*);
  bool LockSlowPath(impl::TaskContext*, Deadline);

  std::atomic<impl::TaskContext*> owner_;
  impl::FastPimplWaitList lock_waiters_;
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
