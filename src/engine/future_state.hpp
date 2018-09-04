#pragma once

#include <atomic>
#include <chrono>
#include <exception>
#include <future>
#include <mutex>

#include <engine/deadline.hpp>
#include <utils/result_store.hpp>

#include "condition_variable_any.hpp"

namespace engine {
namespace impl {

template <typename T>
class FutureState {
 public:
  bool IsReady() const;
  T Get();
  void Wait();

  template <typename Rep, typename Period>
  std::future_status WaitFor(
      const std::chrono::duration<Rep, Period>& duration);

  template <typename Clock, typename Duration>
  std::future_status WaitUntil(
      const std::chrono::time_point<Clock, Duration>& until);

  void EnsureNotRetrieved();

  void SetValue(const T& value);
  void SetValue(T&& value);
  void SetException(std::exception_ptr&& ex);

 private:
  std::mutex mutex_;
  ConditionVariableAny<std::mutex> result_cv_;
  std::atomic<bool> is_ready_{false};
  std::atomic_flag is_retrieved_{ATOMIC_FLAG_INIT};
  utils::ResultStore<T> result_store_;
};

template <>
class FutureState<void> {
 public:
  bool IsReady() const;
  void Get();
  void Wait();

  template <typename Rep, typename Period>
  std::future_status WaitFor(
      const std::chrono::duration<Rep, Period>& duration);

  template <typename Clock, typename Duration>
  std::future_status WaitUntil(
      const std::chrono::time_point<Clock, Duration>& until);

  void EnsureNotRetrieved();

  void SetValue();
  void SetException(std::exception_ptr&& ex);

 private:
  std::mutex mutex_;
  ConditionVariableAny<std::mutex> result_cv_;
  std::atomic<bool> is_ready_{false};
  std::atomic_flag is_retrieved_{ATOMIC_FLAG_INIT};
  utils::ResultStore<void> result_store_;
};

template <typename T>
bool FutureState<T>::IsReady() const {
  return is_ready_;
}

template <typename T>
T FutureState<T>::Get() {
  Wait();
  return result_store_.Retrieve();
}

template <typename T>
void FutureState<T>::Wait() {
  std::unique_lock<std::mutex> lock(mutex_);
  result_cv_.Wait(lock, [this] { return IsReady(); });
}

template <typename T>
template <typename Rep, typename Period>
std::future_status FutureState<T>::WaitFor(
    const std::chrono::duration<Rep, Period>& duration) {
  std::unique_lock<std::mutex> lock(mutex_);
  return result_cv_.WaitUntil(lock, Deadline::FromDuration(duration),
                              [this] { return IsReady(); })
             ? std::future_status::ready
             : std::future_status::timeout;
}

template <typename T>
template <typename Clock, typename Duration>
std::future_status FutureState<T>::WaitUntil(
    const std::chrono::time_point<Clock, Duration>& until) {
  std::unique_lock<std::mutex> lock(mutex_);
  return result_cv_.WaitUntil(lock, Deadline::FromTimePoint(until),
                              [this] { return IsReady(); })
             ? std::future_status::ready
             : std::future_status::timeout;
}

template <typename T>
void FutureState<T>::EnsureNotRetrieved() {
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
    result_store_.SetValue(value);
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
    result_store_.SetValue(std::move(value));
  }
  result_cv_.NotifyAll();
}

template <typename T>
void FutureState<T>::SetException(std::exception_ptr&& ex) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (is_ready_.exchange(true)) {
      throw std::future_error(std::future_errc::promise_already_satisfied);
    }
    result_store_.SetException(std::move(ex));
  }
  result_cv_.NotifyAll();
}

inline bool FutureState<void>::IsReady() const { return is_ready_; }

inline void FutureState<void>::Get() {
  Wait();
  result_store_.Retrieve();
}

inline void FutureState<void>::Wait() {
  std::unique_lock<std::mutex> lock(mutex_);
  result_cv_.Wait(lock, [this] { return IsReady(); });
}

template <typename Rep, typename Period>
std::future_status FutureState<void>::WaitFor(
    const std::chrono::duration<Rep, Period>& duration) {
  std::unique_lock<std::mutex> lock(mutex_);
  return result_cv_.WaitUntil(lock, Deadline::FromDuration(duration),
                              [this] { return IsReady(); })
             ? std::future_status::ready
             : std::future_status::timeout;
}

template <typename Clock, typename Duration>
std::future_status FutureState<void>::WaitUntil(
    const std::chrono::time_point<Clock, Duration>& until) {
  std::unique_lock<std::mutex> lock(mutex_);
  return result_cv_.WaitUntil(lock, Deadline::FromTimePoint(until),
                              [this] { return IsReady(); })
             ? std::future_status::ready
             : std::future_status::timeout;
}

inline void FutureState<void>::EnsureNotRetrieved() {
  if (is_retrieved_.test_and_set()) {
    throw std::future_error(std::future_errc::future_already_retrieved);
  }
}

inline void FutureState<void>::SetValue() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (is_ready_.exchange(true)) {
      throw std::future_error(std::future_errc::promise_already_satisfied);
    }
    result_store_.SetValue();
  }
  result_cv_.NotifyAll();
}

inline void FutureState<void>::SetException(std::exception_ptr&& ex) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (is_ready_.exchange(true)) {
      throw std::future_error(std::future_errc::promise_already_satisfied);
    }
    result_store_.SetException(std::move(ex));
  }
  result_cv_.NotifyAll();
}

}  // namespace impl
}  // namespace engine
