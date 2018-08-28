#pragma once

#include <chrono>
#include <condition_variable>
#include <memory>

#include <engine/deadline.hpp>
#include <engine/mutex.hpp>

namespace engine {

class ConditionVariable {
 public:
  ConditionVariable();
  ~ConditionVariable();

  ConditionVariable(const ConditionVariable&) = delete;
  ConditionVariable(ConditionVariable&&) noexcept;
  ConditionVariable& operator=(const ConditionVariable&) = delete;
  ConditionVariable& operator=(ConditionVariable&&) noexcept;

  void Wait(std::unique_lock<Mutex>& lock);

  template <typename Predicate>
  void Wait(std::unique_lock<Mutex>& lock, Predicate predicate);

  template <typename Rep, typename Period>
  std::cv_status WaitFor(std::unique_lock<Mutex>& lock,
                         const std::chrono::duration<Rep, Period>& timeout);

  template <typename Rep, typename Period, typename Predicate>
  bool WaitFor(std::unique_lock<Mutex>& lock,
               const std::chrono::duration<Rep, Period>& timeout,
               Predicate predicate);

  template <typename Clock, typename Duration>
  std::cv_status WaitUntil(
      std::unique_lock<Mutex>& lock,
      const std::chrono::time_point<Clock, Duration>& until);

  template <typename Clock, typename Duration, typename Predicate>
  bool WaitUntil(std::unique_lock<Mutex>& lock,
                 const std::chrono::time_point<Clock, Duration>& until,
                 Predicate predicate);

  void NotifyOne();
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
