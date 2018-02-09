#pragma once

#include <atomic>
#include <chrono>
#include <exception>
#include <future>
#include <memory>
#include <mutex>

#include "cond_var.hpp"

namespace engine {
namespace impl {

template <typename CurrentTask, typename T>
class FutureState;

}  // namespace impl

template <typename CurrentTask, typename T>
class Promise;

template <typename CurrentTask, typename T>
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
  friend class Promise<CurrentTask, T>;

  explicit Future(
      std::shared_ptr<impl::FutureState<CurrentTask, T>> state);

  void CheckValid() const;

  std::shared_ptr<impl::FutureState<CurrentTask, T>> state_;
};

template <typename CurrentTask, typename T>
class Promise {
 public:
  Promise();
  ~Promise();

  Promise(const Promise&) = delete;
  Promise(Promise&&) noexcept = default;
  Promise& operator=(const Promise&) = delete;
  Promise& operator=(Promise&&) noexcept = default;

  Future<CurrentTask, T> GetFuture();

  void SetValue(const T&);
  void SetValue(T&&);
  void SetException(std::exception_ptr ex);

 private:
  std::shared_ptr<impl::FutureState<CurrentTask, T>> state_;
};

template <typename CurrentTask>
class Promise<CurrentTask, void>;

template <typename CurrentTask>
class Future<CurrentTask, void> {
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
  friend class Promise<CurrentTask, void>;

  explicit Future(
      std::shared_ptr<impl::FutureState<CurrentTask, void>> state);

  void CheckValid() const;

  std::shared_ptr<impl::FutureState<CurrentTask, void>> state_;
};

template <typename CurrentTask>
class Promise<CurrentTask, void> {
 public:
  Promise();
  ~Promise() noexcept(false);

  Promise(const Promise&) = delete;
  Promise(Promise&&) noexcept = default;
  Promise& operator=(const Promise&) = delete;
  Promise& operator=(Promise&&) noexcept = default;

  Future<CurrentTask, void> GetFuture();

  void SetValue();
  void SetException(std::exception_ptr ex);

 private:
  std::shared_ptr<impl::FutureState<CurrentTask, void>> state_;
};

template <typename CurrentTask, typename T>
bool Future<CurrentTask, T>::IsValid() const {
  return !!state_;
}

template <typename CurrentTask, typename T>
T Future<CurrentTask, T>::Get() {
  CheckValid();
  auto result = state_->Get();
  state_.Reset();
  return result;
}

template <typename CurrentTask, typename T>
void Future<CurrentTask, T>::Wait() const {
  CheckValid();
  state_->Wait();
}

template <typename CurrentTask, typename T>
template <typename Rep, typename Period>
std::future_status Future<CurrentTask, T>::WaitFor(
    const std::chrono::duration<Rep, Period>& duration) const {
  CheckValid();
  return state_->WaitFor(duration);
}

template <typename CurrentTask, typename T>
template <typename Clock, typename Duration>
std::future_status Future<CurrentTask, T>::WaitUntil(
    const std::chrono::time_point<Clock, Duration>& until) const {
  CheckValid();
  return state_->WaitUntil(until);
}

template <typename CurrentTask, typename T>
Future<CurrentTask, T>::Future(
    std::shared_ptr<impl::FutureState<CurrentTask, T>> state)
    : state_(std::move(state)) {
  CheckValid();
  state_->EnsureUnique();
}

template <typename CurrentTask, typename T>
void Future<CurrentTask, T>::CheckValid() const {
  if (!state_) {
    throw std::future_error(std::future_errc::no_state);
  }
}

template <typename CurrentTask, typename T>
Promise<CurrentTask, T>::Promise()
    : state_(std::make_shared<impl::FutureState<CurrentTask, T>>()) {}

template <typename CurrentTask, typename T>
Promise<CurrentTask, T>::~Promise() noexcept(false) {
  if (state_ && !state_->IsReady()) {
    throw std::future_error(std::future_errc::broken_promise);
  }
}

template <typename CurrentTask, typename T>
Future<CurrentTask, T> Promise<CurrentTask, T>::GetFuture() {
  return Future<CurrentTask, T>(state_);
}

template <typename CurrentTask, typename T>
void Promise<CurrentTask, T>::SetValue(const T& value) {
  state_->SetValue(value);
}

template <typename CurrentTask, typename T>
void Promise<CurrentTask, T>::SetValue(T&& value) {
  state_->SetValue(std::move(value));
}

template <typename CurrentTask, typename T>
void Promise<CurrentTask, T>::SetException(std::exception_ptr ex) {
  state_->set_exception(ex);
}

template <typename CurrentTask>
bool Future<CurrentTask, void>::IsValid() const {
  return !!state_;
}

template <typename CurrentTask>
void Future<CurrentTask, void>::Get() {
  CheckValid();
  state_->Get();
  state_.Reset();
}

template <typename CurrentTask>
void Future<CurrentTask, void>::Wait() const {
  CheckValid();
  state_->Wait();
}

template <typename CurrentTask>
template <typename Rep, typename Period>
std::future_status Future<CurrentTask, void>::WaitFor(
    const std::chrono::duration<Rep, Period>& duration) const {
  CheckValid();
  return state_->WaitFor(duration);
}

template <typename CurrentTask>
template <typename Clock, typename Duration>
std::future_status Future<CurrentTask, void>::WaitUntil(
    const std::chrono::time_point<Clock, Duration>& until) const {
  CheckValid();
  return state_->WaitUntil(until);
}

template <typename CurrentTask>
Future<CurrentTask, void>::Future(
    std::shared_ptr<impl::FutureState<CurrentTask, void>> state)
    : state_(std::move(state)) {
  CheckValid();
  state_->EnsureUnique();
}

template <typename CurrentTask>
void Future<CurrentTask, void>::CheckValid() const {
  if (!state_) {
    throw std::future_error(std::future_errc::no_state);
  }
}

template <typename CurrentTask>
Promise<CurrentTask, void>::Promise()
    : state_(std::make_shared<impl::FutureState<CurrentTask, void>>()) {}

template <typename CurrentTask>
Promise<CurrentTask, void>::~Promise() noexcept(false) {
  if (state_ && !state_->IsReady()) {
    throw std::future_error(std::future_errc::broken_promise);
  }
}

template <typename CurrentTask>
Future<CurrentTask, void> Promise<CurrentTask, void>::GetFuture() {
  return Future<CurrentTask, void>(state_);
}

template <typename CurrentTask>
void Promise<CurrentTask, void>::SetValue() {
  state_->SetValue();
}

template <typename CurrentTask>
void Promise<CurrentTask, void>::SetException(std::exception_ptr ex) {
  state_->set_exception(ex);
}

namespace impl {

template <typename CurrentTask, typename T>
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
  CondVar<CurrentTask> result_cv_;
  std::atomic<bool> is_ready_;
  std::atomic_flag is_retrieved_;
};

template <typename CurrentTask, typename T>
FutureState<CurrentTask, T>::FutureState()
    : is_ready_(false), is_retrieved_(ATOMIC_FLAG_INIT) {}

template <typename CurrentTask, typename T>
bool FutureState<CurrentTask, T>::IsReady() const {
  return is_ready_;
}

template <typename CurrentTask, typename T>
T FutureState<CurrentTask, T>::Get() {
  Wait();
  if (exception_ptr_) {
    std::rethrow_exception(exception_ptr_);
  }
  auto result = std::move(*value_);
  value_.reset();
  return result;
}

template <typename CurrentTask, typename T>
void FutureState<CurrentTask, T>::Wait() {
  std::unique_lock<std::mutex> lock(mutex_);
  result_cv_.Wait(lock, [this] { return IsReady(); });
}

template <typename CurrentTask, typename T>
template <typename Rep, typename Period>
std::future_status FutureState<CurrentTask, T>::WaitFor(
    const std::chrono::duration<Rep, Period>& duration) const {
  std::unique_lock<std::mutex> lock(mutex_);
  return result_cv_.WaitFor(lock, duration, [this] { return IsReady(); })
             ? std::future_status::ready
             : std::future_status::timeout;
}

template <typename CurrentTask, typename T>
template <typename Clock, typename Duration>
std::future_status FutureState<CurrentTask, T>::WaitUntil(
    const std::chrono::time_point<Clock, Duration>& until) const {
  std::unique_lock<std::mutex> lock(mutex_);
  return result_cv_.WaitUntil(lock, until, [this] { return IsReady(); })
             ? std::future_status::ready
             : std::future_status::timeout;
}

template <typename CurrentTask, typename T>
void FutureState<CurrentTask, T>::EnsureUnique() {
  if (is_retrieved_.test_and_set()) {
    throw std::future_error(std::future_errc::future_already_retrieved);
  }
}

template <typename CurrentTask, typename T>
void FutureState<CurrentTask, T>::SetValue(const T& value) {
  if (is_ready_.exchange(true)) {
    throw std::future_error(std::future_errc::promise_already_satisfied);
  }
  {
    std::lock_guard<std::mutex> lock(mutex_);
    value_ = std::make_unique<T>(value);
  }
  result_cv_.NotifyAll();
}

template <typename CurrentTask, typename T>
void FutureState<CurrentTask, T>::SetValue(T&& value) {
  if (is_ready_.exchange(true)) {
    throw std::future_error(std::future_errc::promise_already_satisfied);
  }
  {
    std::lock_guard<std::mutex> lock(mutex_);
    value_ = std::make_unique<T>(std::move(value));
  }
  result_cv_.NotifyAll();
}

template <typename CurrentTask, typename T>
void FutureState<CurrentTask, T>::SetException(std::exception_ptr ex) {
  if (is_ready_.exchange(true)) {
    throw std::future_error(std::future_errc::promise_already_satisfied);
  }
  {
    std::lock_guard<std::mutex> lock(mutex_);
    exception_ptr_ = ex;
  }
  result_cv_.NotifyAll();
}

template <typename CurrentTask>
class FutureState<CurrentTask, void> {
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
  CondVar<CurrentTask> result_cv_;
  std::atomic<bool> is_ready_;
  std::atomic_flag is_retrieved_;
};

template <typename CurrentTask>
FutureState<CurrentTask, void>::FutureState()
    : is_ready_(false), is_retrieved_(ATOMIC_FLAG_INIT) {}

template <typename CurrentTask>
bool FutureState<CurrentTask, void>::IsReady() const {
  return is_ready_;
}

template <typename CurrentTask>
void FutureState<CurrentTask, void>::Get() {
  Wait();
  if (exception_ptr_) {
    std::rethrow_exception(exception_ptr_);
  }
}

template <typename CurrentTask>
void FutureState<CurrentTask, void>::Wait() {
  std::unique_lock<std::mutex> lock(mutex_);
  result_cv_.Wait(lock, [this] { return IsReady(); });
}

template <typename CurrentTask>
template <typename Rep, typename Period>
std::future_status FutureState<CurrentTask, void>::WaitFor(
    const std::chrono::duration<Rep, Period>& duration) const {
  std::unique_lock<std::mutex> lock(mutex_);
  return result_cv_.WaitFor(lock, duration, [this] { return IsReady(); })
             ? std::future_status::ready
             : std::future_status::timeout;
}

template <typename CurrentTask>
template <typename Clock, typename Duration>
std::future_status FutureState<CurrentTask, void>::WaitUntil(
    const std::chrono::time_point<Clock, Duration>& until) const {
  std::unique_lock<std::mutex> lock(mutex_);
  return result_cv_.WaitUntil(lock, until, [this] { return IsReady(); })
             ? std::future_status::ready
             : std::future_status::timeout;
}

template <typename CurrentTask>
void FutureState<CurrentTask, void>::EnsureUnique() {
  if (is_retrieved_.test_and_set()) {
    throw std::future_error(std::future_errc::future_already_retrieved);
  }
}

template <typename CurrentTask>
void FutureState<CurrentTask, void>::SetValue() {
  if (is_ready_.exchange(true)) {
    throw std::future_error(std::future_errc::promise_already_satisfied);
  }
  result_cv_.NotifyAll();
}

template <typename CurrentTask>
void FutureState<CurrentTask, void>::SetException(std::exception_ptr ex) {
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
}  // namespace engine
