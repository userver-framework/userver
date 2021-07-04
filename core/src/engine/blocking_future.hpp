#pragma once

#include <chrono>
#include <exception>
#include <future>
#include <memory>

#include <userver/utils/assert.hpp>

#include <engine/blocking_future_state.hpp>
#include <userver/engine/deadline.hpp>

namespace engine::impl {

// Convenience classes for asynchronous data/event transfer
// from external world (ev loops) to coroutines.
// Operations on these classes are blocking (they use std::mutex).
//
// BlockingFuture can only be used in coroutines.
// BlockingPromise should not be used from coroutines (use Async instead).

template <typename T>
class BlockingPromise;

template <typename T>
class BlockingFuture {
 public:
  BlockingFuture() = default;

  BlockingFuture(const BlockingFuture&) = delete;
  BlockingFuture(BlockingFuture&&) noexcept = default;
  BlockingFuture& operator=(const BlockingFuture&) = delete;
  BlockingFuture& operator=(BlockingFuture&&) noexcept = default;

  bool valid() const noexcept;

  T get();
  void wait() const;

  template <typename Rep, typename Period>
  std::future_status wait_for(
      std::chrono::duration<Rep, Period> duration) const;

  template <typename Clock, typename Duration>
  std::future_status wait_until(
      std::chrono::time_point<Clock, Duration> until) const;

  std::future_status wait_until(Deadline deadline) const;

 private:
  friend class BlockingPromise<T>;

  explicit BlockingFuture(std::shared_ptr<impl::BlockingFutureState<T>> state);

  void CheckValid() const;

  std::shared_ptr<impl::BlockingFutureState<T>> state_;
};

template <typename T>
class BlockingPromise {
 public:
  BlockingPromise();
  ~BlockingPromise();

  BlockingPromise(const BlockingPromise&) = delete;
  BlockingPromise(BlockingPromise&&) noexcept = default;
  BlockingPromise& operator=(const BlockingPromise&) = delete;
  BlockingPromise& operator=(BlockingPromise&&) noexcept = default;

  [[nodiscard]] BlockingFuture<T> get_future();

  // WARN: These functions will return after future.get(), ensure ownership!
  void set_value(const T&);
  void set_value(T&&);
  void set_exception(std::exception_ptr ex);

 private:
  std::shared_ptr<impl::BlockingFutureState<T>> state_;
};

template <>
class BlockingPromise<void> final {
 public:
  BlockingPromise();
  ~BlockingPromise();

  BlockingPromise(const BlockingPromise&) = delete;
  BlockingPromise(BlockingPromise&&) noexcept = default;
  BlockingPromise& operator=(const BlockingPromise&) = delete;
  BlockingPromise& operator=(BlockingPromise&&) noexcept = default;

  [[nodiscard]] BlockingFuture<void> get_future();

  void set_value();
  void set_exception(std::exception_ptr ex);

 private:
  std::shared_ptr<impl::BlockingFutureState<void>> state_;
};

template <typename T>
bool BlockingFuture<T>::valid() const noexcept {
  return !!state_;
}

template <typename T>
T BlockingFuture<T>::get() {
  CheckValid();
  auto result = std::exchange(state_, nullptr)->Get();
  return result;
}

template <>
inline void BlockingFuture<void>::get() {
  CheckValid();
  std::exchange(state_, nullptr)->Get();
}

template <typename T>
void BlockingFuture<T>::wait() const {
  CheckValid();
  state_->Wait();
}

template <typename T>
template <typename Rep, typename Period>
std::future_status BlockingFuture<T>::wait_for(
    std::chrono::duration<Rep, Period> duration) const {
  return wait_until(Deadline::FromDuration(duration));
}

template <typename T>
template <typename Clock, typename Duration>
std::future_status BlockingFuture<T>::wait_until(
    std::chrono::time_point<Clock, Duration> until) const {
  return wait_until(Deadline::FromTimePoint(until));
}

template <typename T>
std::future_status BlockingFuture<T>::wait_until(Deadline deadline) const {
  CheckValid();
  return state_->WaitUntil(deadline);
}

template <typename T>
BlockingFuture<T>::BlockingFuture(
    std::shared_ptr<impl::BlockingFutureState<T>> state)
    : state_(std::move(state)) {
  CheckValid();
  state_->EnsureNotRetrieved();
}

template <typename T>
void BlockingFuture<T>::CheckValid() const {
  if (!state_) {
    throw std::future_error(std::future_errc::no_state);
  }
}

template <typename T>
BlockingPromise<T>::BlockingPromise()
    : state_(std::make_shared<impl::BlockingFutureState<T>>()) {}

template <typename T>
BlockingPromise<T>::~BlockingPromise() {
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
BlockingFuture<T> BlockingPromise<T>::get_future() {
  return BlockingFuture<T>(state_);
}

template <typename T>
void BlockingPromise<T>::set_value(const T& value) {
  state_->SetValue(value);
}

template <typename T>
void BlockingPromise<T>::set_value(T&& value) {
  state_->SetValue(std::move(value));
}

template <typename T>
void BlockingPromise<T>::set_exception(std::exception_ptr ex) {
  state_->SetException(std::move(ex));
}

inline BlockingPromise<void>::BlockingPromise()
    : state_(std::make_shared<impl::BlockingFutureState<void>>()) {}

inline BlockingPromise<void>::~BlockingPromise() {
  if (state_ && !state_->IsReady()) {
    try {
      state_->SetException(std::make_exception_ptr(
          std::future_error(std::future_errc::broken_promise)));
    } catch (const std::future_error&) {
      UASSERT_MSG(false, "Invalid promise usage");
    }
  }
}

inline BlockingFuture<void> BlockingPromise<void>::get_future() {
  return BlockingFuture<void>(state_);
}

inline void BlockingPromise<void>::set_value() { state_->SetValue(); }

inline void BlockingPromise<void>::set_exception(std::exception_ptr ex) {
  state_->SetException(std::move(ex));
}

}  // namespace engine::impl
