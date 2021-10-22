#pragma once

#include <atomic>
#include <exception>
#include <future>
#include <mutex>

#include <userver/engine/deadline.hpp>
#include <userver/engine/impl/condition_variable_any.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/result_store.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine::impl {

template <typename T>
class BlockingFutureState final {
 public:
  bool IsReady() const noexcept;

  [[nodiscard]] T Get();
  void Wait();
  [[nodiscard]] std::future_status WaitUntil(Deadline deadline);

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
class BlockingFutureState<void> final {
 public:
  bool IsReady() const noexcept;

  void Get();
  void Wait();
  [[nodiscard]] std::future_status WaitUntil(Deadline deadline);

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
bool BlockingFutureState<T>::IsReady() const noexcept {
  return is_ready_.load(std::memory_order_relaxed);
}

template <typename T>
T BlockingFutureState<T>::Get() {
  Wait();
  return result_store_.Retrieve();
}

template <typename T>
void BlockingFutureState<T>::Wait() {
  engine::TaskCancellationBlocker block_cancel;
  std::unique_lock<std::mutex> lock(mutex_);
  [[maybe_unused]] bool is_ready =
      result_cv_.WaitUntil(lock, {}, [this] { return IsReady(); });
  UASSERT(is_ready);
}

template <typename T>
std::future_status BlockingFutureState<T>::WaitUntil(Deadline deadline) {
  std::unique_lock<std::mutex> lock(mutex_);
  return result_cv_.WaitUntil(lock, deadline, [this] { return IsReady(); })
             ? std::future_status::ready
             : std::future_status::timeout;
}

template <typename T>
void BlockingFutureState<T>::EnsureNotRetrieved() {
  if (is_retrieved_.test_and_set()) {
    throw std::future_error(std::future_errc::future_already_retrieved);
  }
}

template <typename T>
void BlockingFutureState<T>::SetValue(const T& value) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (is_ready_.exchange(true, std::memory_order_relaxed)) {
      throw std::future_error(std::future_errc::promise_already_satisfied);
    }
    result_store_.SetValue(value);
  }
  result_cv_.NotifyAll();
}

template <typename T>
void BlockingFutureState<T>::SetValue(T&& value) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (is_ready_.exchange(true, std::memory_order_relaxed)) {
      throw std::future_error(std::future_errc::promise_already_satisfied);
    }
    result_store_.SetValue(std::move(value));
  }
  result_cv_.NotifyAll();
}

template <typename T>
void BlockingFutureState<T>::SetException(std::exception_ptr&& ex) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (is_ready_.exchange(true, std::memory_order_relaxed)) {
      throw std::future_error(std::future_errc::promise_already_satisfied);
    }
    result_store_.SetException(std::move(ex));
  }
  result_cv_.NotifyAll();
}

inline bool BlockingFutureState<void>::IsReady() const noexcept {
  return is_ready_.load(std::memory_order_relaxed);
}

inline void BlockingFutureState<void>::Get() {
  Wait();
  result_store_.Retrieve();
}

inline void BlockingFutureState<void>::Wait() {
  engine::TaskCancellationBlocker block_cancel;
  std::unique_lock<std::mutex> lock(mutex_);
  [[maybe_unused]] bool is_ready =
      result_cv_.WaitUntil(lock, {}, [this] { return IsReady(); });
  UASSERT(is_ready);
}

inline std::future_status BlockingFutureState<void>::WaitUntil(
    Deadline deadline) {
  std::unique_lock<std::mutex> lock(mutex_);
  return result_cv_.WaitUntil(lock, deadline, [this] { return IsReady(); })
             ? std::future_status::ready
             : std::future_status::timeout;
}

inline void BlockingFutureState<void>::EnsureNotRetrieved() {
  if (is_retrieved_.test_and_set(std::memory_order_relaxed)) {
    throw std::future_error(std::future_errc::future_already_retrieved);
  }
}

inline void BlockingFutureState<void>::SetValue() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (is_ready_.exchange(true, std::memory_order_relaxed)) {
      throw std::future_error(std::future_errc::promise_already_satisfied);
    }
    result_store_.SetValue();
  }
  result_cv_.NotifyAll();
}

inline void BlockingFutureState<void>::SetException(std::exception_ptr&& ex) {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (is_ready_.exchange(true, std::memory_order_relaxed)) {
      throw std::future_error(std::future_errc::promise_already_satisfied);
    }
    result_store_.SetException(std::move(ex));
  }
  result_cv_.NotifyAll();
}

}  // namespace engine::impl

USERVER_NAMESPACE_END
