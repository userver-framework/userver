#pragma once

/// @file engine/future.hpp
/// @brief @copybrief engine::Future

#include <chrono>
#include <exception>
#include <memory>

#include <engine/deadline.hpp>
#include <engine/exception.hpp>
#include <engine/future_state.hpp>
#include <utils/assert.hpp>

namespace engine {

/// Allows to store a value to be asynchronously retrieved from the Future
template <typename T>
class Promise;

/// Class for asynchronous value exchange
template <typename T>
class Future {
 public:
  /// Creates an Future without a valid state.
  Future() = default;

  Future(const Future&) = delete;
  Future(Future&&) noexcept = default;
  Future& operator=(const Future&) = delete;
  Future& operator=(Future&&) noexcept = default;

  /// Returns whether this Future holds a valid state.
  bool valid() const noexcept;

  /// @brief Waits for value availability and retrieves it.
  /// @throw WaitInterruptedException if the current task has been cancelled in
  /// the process.
  /// @throw std::future_error if Future holds no state, if the Promise has
  /// been destroyed without setting a value or if the value has already been
  /// retrieved.
  T get();

  /// @brief Waits for value availability.
  /// @returns `FutureStatus::kReady` if the value is available.
  /// @returns `FutureStatus::kCancelled` if current task is being cancelled.
  /// @throw std::future_error if Future holds no state.
  [[nodiscard]] FutureStatus wait() const;

  /// @brief Waits for value availability until the timeout expires or until the
  /// task is cancelled.
  /// @returns `FutureStatus::kReady` if the value is available.
  /// @returns `FutureStatus::kTimeout` if `timeout` has expired.
  /// @returns `FutureStatus::kCancelled` if current task is being cancelled.
  /// @throw std::future_error if Future holds no state.
  template <typename Rep, typename Period>
  FutureStatus wait_for(std::chrono::duration<Rep, Period> timeout) const;

  /// @brief Waits for value availability until the deadline is reached or until
  /// the task is cancelled.
  /// @returns `FutureStatus::kReady` if the value is available.
  /// @returns `FutureStatus::kTimeout` if `until` time point was reached.
  /// @returns `FutureStatus::kCancelled` if current task is being cancelled.
  /// @throw std::future_error if Future holds no state.
  template <typename Clock, typename Duration>
  FutureStatus wait_until(std::chrono::time_point<Clock, Duration> until) const;

  /// @brief Waits for value availability until the deadline is reached or until
  /// the task is cancelled.
  /// @returns `FutureStatus::kReady` if the value is available.
  /// @returns `FutureStatus::kTimeout` if `deadline` was reached.
  /// @returns `FutureStatus::kCancelled` if current task is being cancelled.
  /// @throw std::future_error if Future holds no state.
  FutureStatus wait_until(Deadline deadline) const;

 private:
  friend class Promise<T>;

  explicit Future(std::shared_ptr<impl::FutureState<T>> state);

  void CheckValid() const;

  std::shared_ptr<impl::FutureState<T>> state_;
};

template <typename T>
class Promise {
 public:
  /// Creates a new asynchronous value store.
  Promise();

  ~Promise();

  Promise(const Promise&) = delete;
  Promise(Promise&&) noexcept = default;
  Promise& operator=(const Promise&) = delete;
  Promise& operator=(Promise&&) noexcept = default;

  /// Retrieves the Future associated with this value store.
  /// @throw std::future_error if the Future has already been retrieved.
  [[nodiscard]] Future<T> get_future();

  /// Stores a value for retrieval.
  /// @throw std::future_error if a value or an exception has already been set.
  void set_value(const T&);

  /// Stores a value for retrieval.
  /// @throw std::future_error if a value or an exception has already been set.
  void set_value(T&&);

  /// Stores an exception to be thrown on retrieval.
  /// @throw std::future_error if a value or an exception has already been set.
  void set_exception(std::exception_ptr ex);

 private:
  std::shared_ptr<impl::FutureState<T>> state_;
};

template <>
class Promise<void> final {
 public:
  /// Creates a new asynchronous signal store.
  Promise();

  ~Promise();

  Promise(const Promise&) = delete;
  Promise(Promise&&) noexcept = default;
  Promise& operator=(const Promise&) = delete;
  Promise& operator=(Promise&&) noexcept = default;

  /// Retrieves the Future associated with this signal store.
  /// @throw std::future_error if the Future has already been retrieved.
  [[nodiscard]] Future<void> get_future();

  /// Stores a signal for retrieval.
  /// @throw std::future_error if a signal or an exception has already been set.
  void set_value();

  /// Stores an exception to be thrown on retrieval.
  /// @throw std::future_error if a signal or an exception has already been set.
  void set_exception(std::exception_ptr ex);

 private:
  std::shared_ptr<impl::FutureState<void>> state_;
};

template <typename T>
bool Future<T>::valid() const noexcept {
  return !!state_;
}

template <typename T>
T Future<T>::get() {
  CheckValid();
  auto result = std::exchange(state_, nullptr)->Get();
  return result;
}

template <>
inline void Future<void>::get() {
  CheckValid();
  std::exchange(state_, nullptr)->Get();
}

template <typename T>
FutureStatus Future<T>::wait() const {
  CheckValid();
  return state_->Wait();
}

template <typename T>
template <typename Rep, typename Period>
FutureStatus Future<T>::wait_for(
    std::chrono::duration<Rep, Period> duration) const {
  return wait_until(Deadline::FromDuration(duration));
}

template <typename T>
template <typename Clock, typename Duration>
FutureStatus Future<T>::wait_until(
    std::chrono::time_point<Clock, Duration> until) const {
  return wait_until(Deadline::FromTimePoint(until));
}

template <typename T>
FutureStatus Future<T>::wait_until(Deadline deadline) const {
  CheckValid();
  return state_->WaitUntil(deadline);
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
    // TODO: TAXICOMMON-3186
    // NOLINTNEXTLINE(misc-taxi-coroutine-unsafe)
    throw std::future_error(std::future_errc::no_state);
  }
}

template <typename T>
Promise<T>::Promise() : state_(std::make_shared<impl::FutureState<T>>()) {}

template <typename T>
Promise<T>::~Promise() {
  if (state_ && !state_->IsReady()) {
    try {
      state_->SetException(std::make_exception_ptr(
          // TODO: TAXICOMMON-3186
          // NOLINTNEXTLINE(misc-taxi-coroutine-unsafe)
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

inline Promise<void>::~Promise() {
  if (state_ && !state_->IsReady()) {
    try {
      state_->SetException(std::make_exception_ptr(
          // TODO: TAXICOMMON-3186
          // NOLINTNEXTLINE(misc-taxi-coroutine-unsafe)
          std::future_error(std::future_errc::broken_promise)));
    } catch (const std::future_error&) {
      UASSERT_MSG(false, "Invalid promise usage");
    }
  }
}

inline Future<void> Promise<void>::get_future() { return Future<void>(state_); }

inline void Promise<void>::set_value() { state_->SetValue(); }

inline void Promise<void>::set_exception(std::exception_ptr ex) {
  state_->SetException(std::move(ex));
}

}  // namespace engine
