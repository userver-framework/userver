#pragma once

#include <atomic>
#include <exception>
#include <future>

#include <userver/engine/deadline.hpp>
#include <userver/engine/exception.hpp>
#include <userver/engine/future_status.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/utils/result_store.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

template <typename T>
class FutureState final {
 public:
  bool IsReady() const noexcept;

  [[nodiscard]] T Get();
  [[nodiscard]] FutureStatus Wait();
  [[nodiscard]] FutureStatus WaitUntil(Deadline);

  void EnsureNotRetrieved();

  void SetValue(const T& value);
  void SetValue(T&& value);
  void SetException(std::exception_ptr&& ex);

 private:
  SingleConsumerEvent event_{SingleConsumerEvent::NoAutoReset{}};
  std::atomic<bool> is_ready_{false};
  std::atomic_flag is_retrieved_ ATOMIC_FLAG_INIT;
  utils::ResultStore<T> result_store_;
};

template <>
class FutureState<void> final {
 public:
  bool IsReady() const noexcept;

  void Get();
  [[nodiscard]] FutureStatus Wait();
  [[nodiscard]] FutureStatus WaitUntil(Deadline);

  void EnsureNotRetrieved();

  void SetValue();
  void SetException(std::exception_ptr&& ex);

 private:
  SingleConsumerEvent event_{SingleConsumerEvent::NoAutoReset{}};
  std::atomic<bool> is_ready_{false};
  std::atomic_flag is_retrieved_ ATOMIC_FLAG_INIT;
  utils::ResultStore<void> result_store_;
};

template <typename T>
bool FutureState<T>::IsReady() const noexcept {
  return is_ready_.load(std::memory_order_relaxed);
}

template <typename T>
T FutureState<T>::Get() {
  const auto wait_result = Wait();
  if (wait_result == FutureStatus::kCancelled) {
    throw WaitInterruptedException(current_task::CancellationReason());
  }
  std::atomic_thread_fence(std::memory_order_acquire);
  return result_store_.Retrieve();
}

template <typename T>
FutureStatus FutureState<T>::Wait() {
  return event_.WaitForEvent() ? FutureStatus::kReady
                               : FutureStatus::kCancelled;
}

template <typename T>
FutureStatus FutureState<T>::WaitUntil(Deadline deadline) {
  return event_.WaitForEventUntil(deadline)
             ? FutureStatus::kReady
             : (current_task::ShouldCancel() ? FutureStatus::kCancelled
                                             : FutureStatus::kTimeout);
}

template <typename T>
void FutureState<T>::EnsureNotRetrieved() {
  if (is_retrieved_.test_and_set(std::memory_order_relaxed)) {
    // TODO: TAXICOMMON-3186
    // NOLINTNEXTLINE(misc-taxi-coroutine-unsafe)
    throw std::future_error(std::future_errc::future_already_retrieved);
  }
}

template <typename T>
void FutureState<T>::SetValue(const T& value) {
  if (is_ready_.exchange(true, std::memory_order_relaxed)) {
    // TODO: TAXICOMMON-3186
    // NOLINTNEXTLINE(misc-taxi-coroutine-unsafe)
    throw std::future_error(std::future_errc::promise_already_satisfied);
  }
  result_store_.SetValue(value);
  std::atomic_thread_fence(std::memory_order_release);
  event_.Send();
}

template <typename T>
void FutureState<T>::SetValue(T&& value) {
  if (is_ready_.exchange(true, std::memory_order_relaxed)) {
    // TODO: TAXICOMMON-3186
    // NOLINTNEXTLINE(misc-taxi-coroutine-unsafe)
    throw std::future_error(std::future_errc::promise_already_satisfied);
  }
  result_store_.SetValue(std::move(value));
  std::atomic_thread_fence(std::memory_order_release);
  event_.Send();
}

template <typename T>
void FutureState<T>::SetException(std::exception_ptr&& ex) {
  if (is_ready_.exchange(true, std::memory_order_relaxed)) {
    // TODO: TAXICOMMON-3186
    // NOLINTNEXTLINE(misc-taxi-coroutine-unsafe)
    throw std::future_error(std::future_errc::promise_already_satisfied);
  }
  result_store_.SetException(std::move(ex));
  std::atomic_thread_fence(std::memory_order_release);
  event_.Send();
}

inline bool FutureState<void>::IsReady() const noexcept {
  return is_ready_.load(std::memory_order_relaxed);
}

inline void FutureState<void>::Get() {
  const auto wait_result = Wait();
  if (wait_result == FutureStatus::kCancelled) {
    throw WaitInterruptedException(current_task::CancellationReason());
  }
  std::atomic_thread_fence(std::memory_order_acquire);
  result_store_.Retrieve();
}

inline FutureStatus FutureState<void>::Wait() {
  return event_.WaitForEvent() ? FutureStatus::kReady
                               : FutureStatus::kCancelled;
}

inline FutureStatus FutureState<void>::WaitUntil(Deadline deadline) {
  return event_.WaitForEventUntil(deadline)
             ? FutureStatus::kReady
             : (current_task::ShouldCancel() ? FutureStatus::kCancelled
                                             : FutureStatus::kTimeout);
}

inline void FutureState<void>::EnsureNotRetrieved() {
  if (is_retrieved_.test_and_set()) {
    // TODO: TAXICOMMON-3186
    // NOLINTNEXTLINE(misc-taxi-coroutine-unsafe)
    throw std::future_error(std::future_errc::future_already_retrieved);
  }
}

inline void FutureState<void>::SetValue() {
  if (is_ready_.exchange(true, std::memory_order_relaxed)) {
    // TODO: TAXICOMMON-3186
    // NOLINTNEXTLINE(misc-taxi-coroutine-unsafe)
    throw std::future_error(std::future_errc::promise_already_satisfied);
  }
  result_store_.SetValue();
  std::atomic_thread_fence(std::memory_order_release);
  event_.Send();
}

inline void FutureState<void>::SetException(std::exception_ptr&& ex) {
  if (is_ready_.exchange(true, std::memory_order_relaxed)) {
    // TODO: TAXICOMMON-3186
    // NOLINTNEXTLINE(misc-taxi-coroutine-unsafe)
    throw std::future_error(std::future_errc::promise_already_satisfied);
  }
  result_store_.SetException(std::move(ex));
  std::atomic_thread_fence(std::memory_order_release);
  event_.Send();
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
