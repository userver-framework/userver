#pragma once

#include <atomic>
#include <chrono>
#include <exception>
#include <future>
#include <memory>
#include <mutex>

#include <engine/task/task.hpp>

#include "condition_variable.hpp"

namespace engine {
namespace impl {

template <typename T>
class FutureState;

}  // namespace impl

template <typename T>
class Promise;

template <typename T>
class Future {
 public:
  Future() = default;

  Future(const Future&) = delete;
  Future(Future&&) noexcept = default;
  Future& operator=(const Future&) = delete;
  Future& operator=(Future&&) noexcept = default;

  bool IsValid() const;

  T Get();
  void Wait() const;

  template <typename Rep, typename Period>
  std::future_status WaitFor(
      const std::chrono::duration<Rep, Period>& duration) const;

  template <typename Clock, typename Duration>
  std::future_status WaitUntil(
      const std::chrono::time_point<Clock, Duration>& until) const;

 private:
  friend class Promise<T>;

  explicit Future(std::shared_ptr<impl::FutureState<T>> state);

  void CheckValid() const;

  std::shared_ptr<impl::FutureState<T>> state_;
};

template <typename T>
class Promise {
 public:
  Promise();
  ~Promise();

  Promise(const Promise&) = delete;
  Promise(Promise&&) noexcept = default;
  Promise& operator=(const Promise&) = delete;
  Promise& operator=(Promise&&) noexcept = default;

  Future<T> GetFuture();

  void SetValue(const T&);
  void SetValue(T&&);
  void SetException(std::exception_ptr ex);

 private:
  std::shared_ptr<impl::FutureState<T>> state_;
};

template <>
class Promise<void>;

template <>
class Future<void> {
 public:
  Future() = default;

  Future(const Future&) = delete;
  Future(Future&&) noexcept = default;
  Future& operator=(const Future&) = delete;
  Future& operator=(Future&&) noexcept = default;

  bool IsValid() const;

  void Get();
  void Wait() const;

  template <typename Rep, typename Period>
  std::future_status WaitFor(
      const std::chrono::duration<Rep, Period>& duration) const;

  template <typename Clock, typename Duration>
  std::future_status WaitUntil(
      const std::chrono::time_point<Clock, Duration>& until) const;

 private:
  friend class Promise<void>;

  explicit Future(std::shared_ptr<impl::FutureState<void>> state);

  void CheckValid() const;

  std::shared_ptr<impl::FutureState<void>> state_;
};

template <>
class Promise<void> {
 public:
  Promise();
  ~Promise() noexcept(false);

  Promise(const Promise&) = delete;
  Promise(Promise&&) noexcept = default;
  Promise& operator=(const Promise&) = delete;
  Promise& operator=(Promise&&) noexcept = default;

  Future<void> GetFuture();

  void SetValue();
  void SetException(std::exception_ptr ex);

 private:
  std::shared_ptr<impl::FutureState<void>> state_;
};

template <typename T>
bool Future<T>::IsValid() const {
  return !!state_;
}

template <typename T>
T Future<T>::Get() {
  CheckValid();
  auto result = state_->Get();
  state_.Reset();
  return result;
}

template <typename T>
void Future<T>::Wait() const {
  CheckValid();
  state_->Wait();
}

template <typename T>
template <typename Rep, typename Period>
std::future_status Future<T>::WaitFor(
    const std::chrono::duration<Rep, Period>& duration) const {
  CheckValid();
  return state_->WaitFor(duration);
}

template <typename T>
template <typename Clock, typename Duration>
std::future_status Future<T>::WaitUntil(
    const std::chrono::time_point<Clock, Duration>& until) const {
  CheckValid();
  return state_->WaitUntil(until);
}

template <typename T>
Future<T>::Future(std::shared_ptr<impl::FutureState<T>> state)
    : state_(std::move(state)) {
  CheckValid();
  state_->EnsureUnique();
}

template <typename T>
void Future<T>::CheckValid() const {
  if (!state_) {
    throw std::future_error(std::future_errc::no_state);
  }
}

template <typename T>
Promise<T>::Promise() : state_(std::make_shared<impl::FutureState<T>>()) {}

template <typename T>
Promise<T>::~Promise() noexcept(false) {
  if (state_ && !state_->IsReady()) {
    throw std::future_error(std::future_errc::broken_promise);
  }
}

template <typename T>
Future<T> Promise<T>::GetFuture() {
  return Future<T>(state_);
}

template <typename T>
void Promise<T>::SetValue(const T& value) {
  state_->SetValue(value);
}

template <typename T>
void Promise<T>::SetValue(T&& value) {
  state_->SetValue(std::move(value));
}

template <typename T>
void Promise<T>::SetException(std::exception_ptr ex) {
  state_->set_exception(ex);
}

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
  if (is_ready_.exchange(true)) {
    throw std::future_error(std::future_errc::promise_already_satisfied);
  }
  {
    std::lock_guard<std::mutex> lock(mutex_);
    value_ = std::make_unique<T>(value);
  }
  result_cv_.NotifyAll();
}

template <typename T>
void FutureState<T>::SetValue(T&& value) {
  if (is_ready_.exchange(true)) {
    throw std::future_error(std::future_errc::promise_already_satisfied);
  }
  {
    std::lock_guard<std::mutex> lock(mutex_);
    value_ = std::make_unique<T>(std::move(value));
  }
  result_cv_.NotifyAll();
}

template <typename T>
void FutureState<T>::SetException(std::exception_ptr ex) {
  if (is_ready_.exchange(true)) {
    throw std::future_error(std::future_errc::promise_already_satisfied);
  }
  {
    std::lock_guard<std::mutex> lock(mutex_);
    exception_ptr_ = ex;
  }
  result_cv_.NotifyAll();
}

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
  if (is_ready_.exchange(true)) {
    throw std::future_error(std::future_errc::promise_already_satisfied);
  }
  result_cv_.NotifyAll();
}

void FutureState<void>::SetException(std::exception_ptr ex) {
  if (is_ready_.exchange(true)) {
    throw std::future_error(std::future_errc::promise_already_satisfied);
  }
  {
    std::lock_guard<std::mutex> lock(mutex_);
    exception_ptr_ = ex;
  }
  result_cv_.NotifyAll();
}

}  // namespace impl

bool Future<void>::IsValid() const { return !!state_; }

void Future<void>::Get() {
  CheckValid();
  state_->Get();
  state_.reset();
}

void Future<void>::Wait() const {
  CheckValid();
  state_->Wait();
}

template <typename Rep, typename Period>
std::future_status Future<void>::WaitFor(
    const std::chrono::duration<Rep, Period>& duration) const {
  CheckValid();
  return state_->WaitFor(duration);
}

template <typename Clock, typename Duration>
std::future_status Future<void>::WaitUntil(
    const std::chrono::time_point<Clock, Duration>& until) const {
  CheckValid();
  return state_->WaitUntil(until);
}

Future<void>::Future(std::shared_ptr<impl::FutureState<void>> state)
    : state_(std::move(state)) {
  CheckValid();
  state_->EnsureUnique();
}

void Future<void>::CheckValid() const {
  if (!state_) {
    throw std::future_error(std::future_errc::no_state);
  }
}

Promise<void>::Promise()
    : state_(std::make_shared<impl::FutureState<void>>()) {}

Promise<void>::~Promise() noexcept(false) {
  if (state_ && !state_->IsReady()) {
    throw std::future_error(std::future_errc::broken_promise);
  }
}

Future<void> Promise<void>::GetFuture() { return Future<void>(state_); }

void Promise<void>::SetValue() { state_->SetValue(); }

void Promise<void>::SetException(std::exception_ptr ex) {
  state_->SetException(ex);
}

}  // namespace engine
