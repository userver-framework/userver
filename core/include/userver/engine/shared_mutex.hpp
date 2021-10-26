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
/// @brief std::shared_mutex replacement for asynchronous tasks
///
/// ## Example usage:
///
/// @snippet engine/shared_mutex_test.cpp  Sample engine::SharedMutex usage
///
/// @see @ref md_en_userver_synchronization
class SharedMutex final {
 public:
  SharedMutex();
  ~SharedMutex() = default;

  SharedMutex(const SharedMutex&) = delete;
  SharedMutex(SharedMutex&&) = delete;
  SharedMutex& operator=(const SharedMutex&) = delete;
  SharedMutex& operator=(SharedMutex&&) = delete;

  void lock();
  void unlock();

  bool try_lock();

  template <typename Rep, typename Period>
  bool try_lock_for(const std::chrono::duration<Rep, Period>&);

  template <typename Clock, typename Duration>
  bool try_lock_until(const std::chrono::time_point<Clock, Duration>&);

  bool try_lock_until(Deadline deadline);

  void lock_shared();
  void unlock_shared();
  bool try_lock_shared();

  template <typename Rep, typename Period>
  bool try_lock_shared_for(const std::chrono::duration<Rep, Period>&);

  template <typename Clock, typename Duration>
  bool try_lock_shared_until(const std::chrono::time_point<Clock, Duration>&);

  bool try_lock_shared_until(Deadline deadline);

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
