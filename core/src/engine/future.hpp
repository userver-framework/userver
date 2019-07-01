#pragma once

#include <chrono>
#include <exception>
#include <future>
#include <memory>

#include "future_state.hpp"

#include <utils/assert.hpp>

namespace engine {

// Convenience classes for asynchronous data/event transfer
// from external world (ev loops) to coroutines.
//
// Future can only be used in coroutines.
// Promise should not be used from coroutines (use Async instead).

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

  T get();
  void wait() const;

  template <typename Rep, typename Period>
  std::future_status wait_for(
      const std::chrono::duration<Rep, Period>& duration) const;

  template <typename Clock, typename Duration>
  std::future_status wait_until(
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
  ~Promise() noexcept;

  Promise(const Promise&) = delete;
  Promise(Promise&&) noexcept = default;
  Promise& operator=(const Promise&) = delete;
  Promise& operator=(Promise&&) noexcept = default;

  Future<T> get_future();

  void set_value(const T&);
  void set_value(T&&);
  void set_exception(std::exception_ptr ex);

 private:
  std::shared_ptr<impl::FutureState<T>> state_;
};

template <>
class Promise<void> final {
 public:
  Promise();
  ~Promise() noexcept;

  Promise(const Promise&) = delete;
  Promise(Promise&&) noexcept = default;
  Promise& operator=(const Promise&) = delete;
  Promise& operator=(Promise&&) noexcept = default;

  Future<void> get_future();

  void set_value();
  void set_exception(std::exception_ptr ex);

 private:
  std::shared_ptr<impl::FutureState<void>> state_;
};

template <typename T>
bool Future<T>::IsValid() const {
  return !!state_;
}

template <typename T>
T Future<T>::get() {
  CheckValid();
  auto result = state_->Get();
  state_.reset();
  return result;
}

template <>
inline void Future<void>::get() {
  CheckValid();
  state_->Get();
  state_.reset();
}

template <typename T>
void Future<T>::wait() const {
  CheckValid();
  state_->Wait();
}

template <typename T>
template <typename Rep, typename Period>
std::future_status Future<T>::wait_for(
    const std::chrono::duration<Rep, Period>& duration) const {
  CheckValid();
  return state_->WaitFor(duration);
}

template <typename T>
template <typename Clock, typename Duration>
std::future_status Future<T>::wait_until(
    const std::chrono::time_point<Clock, Duration>& until) const {
  CheckValid();
  return state_->WaitUntil(until);
}

template <typename T>
Future<T>::Future(std::shared_ptr<impl::FutureState<T>> state)
    : state_(std::move(state)) {
  CheckValid();
  state_->EnsureNotRetrieved();
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
Promise<T>::~Promise() noexcept {
  if (state_ && !state_->IsReady()) {
    try {
      state_->SetException(std::make_exception_ptr(
          std::future_error(std::future_errc::broken_promise)));
    } catch (const std::future_error&) {
      UASSERT_MSG(false, "Invalid promise usage");
    }
  }
}

template <typename T>
Future<T> Promise<T>::get_future() {
  return Future<T>(state_);
}

template <typename T>
void Promise<T>::set_value(const T& value) {
  state_->SetValue(value);
}

template <typename T>
void Promise<T>::set_value(T&& value) {
  state_->SetValue(std::move(value));
}

template <typename T>
void Promise<T>::set_exception(std::exception_ptr ex) {
  state_->SetException(std::move(ex));
}

inline Promise<void>::Promise()
    : state_(std::make_shared<impl::FutureState<void>>()) {}

inline Promise<void>::~Promise() noexcept {
  if (state_ && !state_->IsReady()) {
    try {
      state_->SetException(std::make_exception_ptr(
          std::future_error(std::future_errc::broken_promise)));
    } catch (const std::future_error&) {
      UASSERT_MSG(false, "Invalid promise usage");
    }
  }
}

inline Future<void> Promise<void>::get_future() { return Future<void>(state_); }

inline void Promise<void>::set_value() {
  UASSERT(!state_->IsReady());
  state_->SetValue();
}

inline void Promise<void>::set_exception(std::exception_ptr ex) {
  state_->SetException(std::move(ex));
}

}  // namespace engine
