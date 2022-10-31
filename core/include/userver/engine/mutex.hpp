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
/// @brief std::mutex replacement for asynchronous tasks
///
/// ## Example usage:
///
/// @snippet engine/mutex_test.cpp  Sample engine::Mutex usage
///
/// @see @ref md_en_userver_synchronization
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
  /// any specific order (e.g. FIFO) is incorrect and must be fixed.
  void unlock();

  bool try_lock();

  template <typename Rep, typename Period>
  bool try_lock_for(const std::chrono::duration<Rep, Period>&);

  template <typename Clock, typename Duration>
  bool try_lock_until(const std::chrono::time_point<Clock, Duration>&);

  bool try_lock_until(Deadline deadline);

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
