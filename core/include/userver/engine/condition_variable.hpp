#pragma once

/// @file userver/engine/condition_variable.hpp
/// @brief @copybrief engine::ConditionVariable

#include <chrono>
#include <memory>

#include <userver/engine/condition_variable_status.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/impl/condition_variable_any.hpp>
#include <userver/engine/mutex.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

/// @ingroup userver_concurrency
///
/// @brief std::condition_variable replacement for asynchronous tasks
///
/// ## Example usage:
///
/// @snippet engine/condition_variable_test.cpp  Sample ConditionVariable usage
///
/// @see @ref md_en_userver_synchronization
class ConditionVariable final {
 public:
  ConditionVariable();
  ~ConditionVariable();

  ConditionVariable(const ConditionVariable&) = delete;
  ConditionVariable(ConditionVariable&&) = delete;
  ConditionVariable& operator=(const ConditionVariable&) = delete;
  ConditionVariable& operator=(ConditionVariable&&) = delete;

  /// Suspends execution until notified or cancelled
  /// @returns `CvStatus::kNoTimeout` if variable was notified
  /// @returns `CvStatus::kCancelled` if current task is being cancelled
  [[nodiscard]] CvStatus Wait(std::unique_lock<Mutex>& lock);

  /// @brief Suspends execution until the predicate is `true` when notification
  /// is received or the task is cancelled
  /// @returns the value of the predicate
  template <typename Predicate>
  [[nodiscard]] bool Wait(std::unique_lock<Mutex>& lock, Predicate&& predicate);

  /// @brief Suspends execution until notified or until the timeout expires or
  /// until the task is cancelled.
  /// @returns `CvStatus::kNoTimeout` if variable was notified
  /// @returns `CvStatus::kTimeout` if `timeout` has expired
  /// @returns `CvStatus::kCancelled` if current task is being cancelled
  template <typename Rep, typename Period>
  CvStatus WaitFor(std::unique_lock<Mutex>& lock,
                   std::chrono::duration<Rep, Period> timeout);

  /// @brief Suspends execution until the predicate is `true` when notified
  /// or the timeout expires or the task is cancelled.
  /// @returns the value of the predicate
  template <typename Rep, typename Period, typename Predicate>
  bool WaitFor(std::unique_lock<Mutex>& lock,
               std::chrono::duration<Rep, Period> timeout,
               Predicate&& predicate);

  /// @brief Suspends execution until notified or the time point is reached or
  /// the task is cancelled.
  /// @returns `CvStatus::kNoTimeout` if variable was notified
  /// @returns `CvStatus::kTimeout` if `until` time point was reached
  /// @returns `CvStatus::kCancelled` if current task is being cancelled
  template <typename Clock, typename Duration>
  CvStatus WaitUntil(std::unique_lock<Mutex>& lock,
                     std::chrono::time_point<Clock, Duration> until);

  /// @brief Suspends execution until notified or the deadline is reached or the
  /// task is cancelled.
  /// @returns `CvStatus::kNoTimeout` if variable was notified
  /// @returns `CvStatus::kTimeout` if deadline was reached
  /// @returns `CvStatus::kCancelled` if current task is being cancelled
  CvStatus WaitUntil(std::unique_lock<Mutex>& lock, Deadline deadline);

  /// @brief Suspends execution until the predicate is `true` when notified
  /// or the time point is reached or the task is cancelled.
  /// @returns the value of the predicate
  template <typename Clock, typename Duration, typename Predicate>
  bool WaitUntil(std::unique_lock<Mutex>& lock,
                 std::chrono::time_point<Clock, Duration> until,
                 Predicate&& predicate);

  /// @brief Suspends execution until the predicate is `true` when notified
  /// or the deadline is reached or the task is cancelled.
  /// @returns the value of the predicate
  template <typename Predicate>
  bool WaitUntil(std::unique_lock<Mutex>& lock, Deadline deadline,
                 Predicate&& predicate);

  /// Notifies one of the waiting tasks
  void NotifyOne();

  /// Notifies all waiting tasks
  void NotifyAll();

 private:
  impl::ConditionVariableAny<Mutex> impl_;
};

template <typename Predicate>
bool ConditionVariable::Wait(std::unique_lock<Mutex>& lock,
                             Predicate&& predicate) {
  return WaitUntil(lock, {}, std::forward<Predicate>(predicate));
}

template <typename Rep, typename Period>
CvStatus ConditionVariable::WaitFor(
    std::unique_lock<Mutex>& lock, std::chrono::duration<Rep, Period> timeout) {
  return WaitUntil(lock, Deadline::FromDuration(timeout));
}

template <typename Rep, typename Period, typename Predicate>
bool ConditionVariable::WaitFor(std::unique_lock<Mutex>& lock,
                                std::chrono::duration<Rep, Period> timeout,
                                Predicate&& predicate) {
  return WaitUntil(lock, Deadline::FromDuration(timeout),
                   std::forward<Predicate>(predicate));
}

template <typename Clock, typename Duration>
CvStatus ConditionVariable::WaitUntil(
    std::unique_lock<Mutex>& lock,
    std::chrono::time_point<Clock, Duration> until) {
  return WaitUntil(lock, Deadline::FromTimePoint(until));
}

template <typename Clock, typename Duration, typename Predicate>
bool ConditionVariable::WaitUntil(
    std::unique_lock<Mutex>& lock,
    std::chrono::time_point<Clock, Duration> until, Predicate&& predicate) {
  return WaitUntil(lock, Deadline::FromTimePoint(until),
                   std::forward<Predicate>(predicate));
}

template <typename Predicate>
bool ConditionVariable::WaitUntil(std::unique_lock<Mutex>& lock,
                                  Deadline deadline, Predicate&& predicate) {
  return impl_.WaitUntil(lock, deadline, std::forward<Predicate>(predicate));
}

}  // namespace engine

USERVER_NAMESPACE_END
