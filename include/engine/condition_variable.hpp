#pragma once

/// @file engine/condition_variable.hpp
/// @brief @copybrief engine::ConditionVariable

#include <chrono>
#include <condition_variable>
#include <memory>

#include <engine/deadline.hpp>
#include <engine/mutex.hpp>

namespace engine {

/// std::condition_variable replacement for asynchronous tasks
class ConditionVariable {
 public:
  ConditionVariable();
  ~ConditionVariable();

  ConditionVariable(const ConditionVariable&) = delete;
  ConditionVariable(ConditionVariable&&) noexcept;
  ConditionVariable& operator=(const ConditionVariable&) = delete;
  ConditionVariable& operator=(ConditionVariable&&) noexcept;

  /// Suspends execution until notified
  void Wait(std::unique_lock<Mutex>& lock);

  /// @brief Suspends execution until the predicate is `true` when notification
  /// is received
  template <typename Predicate>
  void Wait(std::unique_lock<Mutex>& lock, Predicate predicate);

  /// @brief Suspends execution until notified or after the specified timeout
  /// @returns `std::cv_status::no_timeout` if variable was notified
  /// @returns `std::cv_status::timeout` if `timeout` has expired
  template <typename Rep, typename Period>
  std::cv_status WaitFor(std::unique_lock<Mutex>& lock,
                         const std::chrono::duration<Rep, Period>& timeout);

  /// @brief Suspends execution until the predicate is `true` when notified
  /// or until the timeout is expired
  /// @returns the value of the predicate
  template <typename Rep, typename Period, typename Predicate>
  bool WaitFor(std::unique_lock<Mutex>& lock,
               const std::chrono::duration<Rep, Period>& timeout,
               Predicate predicate);

  /// @brief Suspends execution until notified or the specified time point is
  /// reached
  /// @returns `std::cv_status::no_timeout` if variable was notified
  /// @returns `std::cv_status::timeout` if `until` time point was reached
  template <typename Clock, typename Duration>
  std::cv_status WaitUntil(
      std::unique_lock<Mutex>& lock,
      const std::chrono::time_point<Clock, Duration>& until);

  /// @brief Suspends execution until the predicate is `true` when notified
  /// or until the time point is reached
  /// @returns the value of the predicate
  template <typename Clock, typename Duration, typename Predicate>
  bool WaitUntil(std::unique_lock<Mutex>& lock,
                 const std::chrono::time_point<Clock, Duration>& until,
                 Predicate predicate);

  /// Notifies one of the waiting tasks
  void NotifyOne();

  /// Notifies all waiting tasks
  void NotifyAll();

 private:
  class Impl;

  void DoWait(std::unique_lock<Mutex>& lock);

  std::cv_status DoWaitUntil(std::unique_lock<Mutex>& lock, Deadline deadline);

  std::unique_ptr<Impl> impl_;
};

inline void ConditionVariable::Wait(std::unique_lock<Mutex>& lock) {
  DoWait(lock);
}

template <typename Predicate>
void ConditionVariable::Wait(std::unique_lock<Mutex>& lock,
                             Predicate predicate) {
  while (!predicate()) {
    DoWait(lock);
  }
}

template <typename Rep, typename Period>
std::cv_status ConditionVariable::WaitFor(
    std::unique_lock<Mutex>& lock,
    const std::chrono::duration<Rep, Period>& timeout) {
  return WaitUntil(lock, MakeDeadline(timeout));
}

template <typename Rep, typename Period, typename Predicate>
bool ConditionVariable::WaitFor(
    std::unique_lock<Mutex>& lock,
    const std::chrono::duration<Rep, Period>& timeout, Predicate predicate) {
  return WaitUntil(lock, MakeDeadline(timeout), std::move(predicate));
}

template <typename Clock, typename Duration>
std::cv_status ConditionVariable::WaitUntil(
    std::unique_lock<Mutex>& lock,
    const std::chrono::time_point<Clock, Duration>& until) {
  return DoWaitUntil(lock, MakeDeadline(until));
}

template <typename Clock, typename Duration, typename Predicate>
bool ConditionVariable::WaitUntil(
    std::unique_lock<Mutex>& lock,
    const std::chrono::time_point<Clock, Duration>& until,
    Predicate predicate) {
  const auto deadline = MakeDeadline(until);
  bool predicate_result = predicate();
  auto status = std::cv_status::no_timeout;
  while (!predicate_result && status == std::cv_status::no_timeout) {
    status = DoWaitUntil(lock, deadline);
    predicate_result = predicate();
  }
  return predicate_result;
}

}  // namespace engine
