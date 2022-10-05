#pragma once

/// @file userver/engine/single_waiter_mutex.hpp
/// @brief @copybrief engine::SingleWaiterMutex

#include <atomic>
#include <chrono>
#include <mutex>  // for std locks

#include <userver/engine/deadline.hpp>
#include <userver/engine/impl/wait_list_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

/// @ingroup userver_concurrency
///
/// @brief There are some situations when a resource is accessed
/// concurrently, but concurrency factor is limited by 2,
/// (for instance: implications of socket r/w duality)
///  SingleWaiterMutex is lighter version of Mutex.
///  At most 1 waiter can be at any given moment
///
/// ## Example usage:
///
/// @snippet engine/single_waiter_mutex_test.cpp  Sample
/// engine::SingleWaiterMutex usage
///
/// @see @ref md_en_userver_synchronization
class SingleWaiterMutex final {
 public:
  SingleWaiterMutex();
  ~SingleWaiterMutex();

  SingleWaiterMutex(const SingleWaiterMutex&) = delete;
  SingleWaiterMutex(SingleWaiterMutex&&) = delete;
  SingleWaiterMutex& operator=(const SingleWaiterMutex&) = delete;
  SingleWaiterMutex& operator=(SingleWaiterMutex&&) = delete;

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
  struct Impl;

  utils::FastPimpl<Impl, 32, 16> impl_;
};

template <typename Rep, typename Period>
bool SingleWaiterMutex::try_lock_for(
    const std::chrono::duration<Rep, Period>& duration) {
  return try_lock_until(Deadline::FromDuration(duration));
}

template <typename Clock, typename Duration>
bool SingleWaiterMutex::try_lock_until(
    const std::chrono::time_point<Clock, Duration>& until) {
  return try_lock_until(Deadline::FromTimePoint(until));
}

}  // namespace engine

USERVER_NAMESPACE_END
