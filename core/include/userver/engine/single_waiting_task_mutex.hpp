#pragma once

/// @file userver/engine/single_waiter_mutex.hpp
/// @brief @copybrief engine::SingleWaitingTaskMutex

#include <atomic>
#include <chrono>
#include <mutex>  // for std locks

#include <userver/engine/deadline.hpp>
#include <userver/engine/impl/wait_list_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

/// @ingroup userver_concurrency
///
/// @brief Lighter version of Mutex with not more than 1 waiting task.
///
/// There are some situations when a resource is accessed
/// concurrently, but concurrency factor is limited by 2.
/// For instance: implications of socket r/w duality
///
/// ## Example usage:
///
/// The class's API is the same as of engine::Mutex:
///
/// @snippet engine/mutex_test.cpp  Sample engine::Mutex usage
///
/// @see @ref md_en_userver_synchronization
class SingleWaitingTaskMutex final {
 public:
  SingleWaitingTaskMutex();
  ~SingleWaitingTaskMutex();

  SingleWaitingTaskMutex(const SingleWaitingTaskMutex&) = delete;
  SingleWaitingTaskMutex(SingleWaitingTaskMutex&&) = delete;
  SingleWaitingTaskMutex& operator=(const SingleWaitingTaskMutex&) = delete;
  SingleWaitingTaskMutex& operator=(SingleWaitingTaskMutex&&) = delete;

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
  void unlock();

  bool try_lock();

  template <typename Rep, typename Period>
  bool try_lock_for(const std::chrono::duration<Rep, Period>&);

  template <typename Clock, typename Duration>
  bool try_lock_until(const std::chrono::time_point<Clock, Duration>&);

  bool try_lock_until(Deadline deadline);

 private:
  class Impl;

  utils::FastPimpl<Impl, 32, 16> impl_;
};

template <typename Rep, typename Period>
bool SingleWaitingTaskMutex::try_lock_for(
    const std::chrono::duration<Rep, Period>& duration) {
  return try_lock_until(Deadline::FromDuration(duration));
}

template <typename Clock, typename Duration>
bool SingleWaitingTaskMutex::try_lock_until(
    const std::chrono::time_point<Clock, Duration>& until) {
  return try_lock_until(Deadline::FromTimePoint(until));
}

}  // namespace engine

USERVER_NAMESPACE_END
