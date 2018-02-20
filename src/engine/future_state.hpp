#pragma once

#include <atomic>
#include <chrono>
#include <exception>
#include <future>
#include <memory>
#include <mutex>

#include "condition_variable.hpp"

namespace engine {
namespace impl {

template <typename T>
class FutureState {
 public:
  FutureState();

  bool IsReady() const;
  T Get();
  void Wait();

  template <typename Rep, typename Period>
  std::future_status WaitFor(
      const std::chrono::duration<Rep, Period>& duration) const;

  template <typename Clock, typename Duration>
  std::future_status WaitUntil(
      const std::chrono::time_point<Clock, Duration>& until) const;

  void EnsureUnique();

  void SetValue(const T& value);
  void SetValue(T&& value);
  void SetException(std::exception_ptr ex);

 private:
  mutable std::mutex mutex_;
  std::unique_ptr<T> value_;
  std::exception_ptr exception_ptr_;
  ConditionVariable result_cv_;
  std::atomic<bool> is_ready_;
  std::atomic_flag is_retrieved_;
};

template <>
class FutureState<void> {
 public:
  FutureState();

  bool IsReady() const;
  void Get();
  void Wait();

  template <typename Rep, typename Period>
  std::future_status WaitFor(
      const std::chrono::duration<Rep, Period>& duration) const;

  template <typename Clock, typename Duration>
  std::future_status WaitUntil(
      const std::chrono::time_point<Clock, Duration>& until) const;

  void EnsureUnique();

  void SetValue();
  void SetException(std::exception_ptr ex);

 private:
  mutable std::mutex mutex_;
  std::exception_ptr exception_ptr_;
  ConditionVariable result_cv_;
  std::atomic<bool> is_ready_;
  std::atomic_flag is_retrieved_;
};

template <typename T>
FutureState<T>::FutureState()
    : is_ready_(false), is_retrieved_(ATOMIC_FLAG_INIT) {}

template <typename T>
bool FutureState<T>::IsReady() const {
  return is_ready_;
}

template <typename T>
T FutureState<T>::Get() {
  Wait();
  if (exception_ptr_) {
    std::rethrow_exception(exception_ptr_);
  }
  auto result = std::move(*value_);
  value_.reset();
  return result;
}

template <typename T>
void FutureState<T>::Wait() {
  std::unique_lock<std::mutex> lock(mutex_);
  result_cv_.Wait(lock, [this] { return IsReady(); });
}

template <typename T>
template <typename Rep, typename Period>
std::future_status FutureState<T>::WaitFor(
    const std::chrono::duration<Rep, Period>& duration) const {
  std::unique_lock<std::mutex> lock(mutex_);
  return result_cv_.WaitFor(lock, duration, [this] { return IsReady(); })
             ? std::future_status::ready
             : std::future_status::timeout;
}

template <typename T>
template <typename Clock, typename Duration>
std::future_status FutureState<T>::WaitUntil(
    const std::chrono::time_point<Clock, Duration>& until) const {
  std::unique_lock<std::mutex> lock(mutex_);
  return result_cv_.WaitUntil(lock, until, [this] { return IsReady(); })
             ? std::future_status::ready
             : std::future_status::timeout;
}

template <typename T>
void FutureState<T>::EnsureUnique() {
  if (is_retrieved_.test_and_set()) {
    throw std::future_error(std::future_errc::future_already_retrieved);
  }
}

template <typename T>
void FutureState<T>::SetValue(const T& value) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (is_ready_.exchange(true)) {
      throw std::future_error(std::future_errc::promise_already_satisfied);
    }
    value_ = std::make_unique<T>(value);
  }
  result_cv_.NotifyAll();
}

template <typename T>
void FutureState<T>::SetValue(T&& value) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (is_ready_.exchange(true)) {
      throw std::future_error(std::future_errc::promise_already_satisfied);
    }
    value_ = std::make_unique<T>(std::move(value));
  }
  result_cv_.NotifyAll();
}

template <typename T>
void FutureState<T>::SetException(std::exception_ptr ex) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (is_ready_.exchange(true)) {
      throw std::future_error(std::future_errc::promise_already_satisfied);
    }
    exception_ptr_ = ex;
  }
  result_cv_.NotifyAll();
}

FutureState<void>::FutureState()
    : is_ready_(false), is_retrieved_(ATOMIC_FLAG_INIT) {}

bool FutureState<void>::IsReady() const { return is_ready_; }

void FutureState<void>::Get() {
  Wait();
  if (exception_ptr_) {
    std::rethrow_exception(exception_ptr_);
  }
}

void FutureState<void>::Wait() {
  std::unique_lock<std::mutex> lock(mutex_);
  result_cv_.Wait(lock, [this] { return IsReady(); });
}

template <typename Rep, typename Period>
std::future_status FutureState<void>::WaitFor(
    const std::chrono::duration<Rep, Period>& duration) const {
  std::unique_lock<std::mutex> lock(mutex_);
  return result_cv_.WaitFor(lock, duration, [this] { return IsReady(); })
             ? std::future_status::ready
             : std::future_status::timeout;
}

template <typename Clock, typename Duration>
std::future_status FutureState<void>::WaitUntil(
    const std::chrono::time_point<Clock, Duration>& until) const {
  std::unique_lock<std::mutex> lock(mutex_);
  return result_cv_.WaitUntil(lock, until, [this] { return IsReady(); })
             ? std::future_status::ready
             : std::future_status::timeout;
}

void FutureState<void>::EnsureUnique() {
  if (is_retrieved_.test_and_set()) {
    throw std::future_error(std::future_errc::future_already_retrieved);
  }
}

void FutureState<void>::SetValue() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (is_ready_.exchange(true)) {
      throw std::future_error(std::future_errc::promise_already_satisfied);
    }
  }
  result_cv_.NotifyAll();
}

void FutureState<void>::SetException(std::exception_ptr ex) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (is_ready_.exchange(true)) {
      throw std::future_error(std::future_errc::promise_already_satisfied);
    }
    exception_ptr_ = ex;
  }
  result_cv_.NotifyAll();
}

}  // namespace impl
}  // namespace engine
