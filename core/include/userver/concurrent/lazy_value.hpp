#pragma once

/// @file userver/utils/lazy_value.hpp
/// @brief @copybrief utils::LazyValue

#include <atomic>
#include <exception>
#include <utility>

#include <userver/engine/condition_variable.hpp>
#include <userver/engine/exception.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/result_store.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent {

/// @brief lazy value computation with multiple users
template <typename T>
class LazyValue final {
 public:
  explicit LazyValue(std::function<T()> f) : f_(std::move(f)) { UASSERT(f_); }

  /// @brief Get an already calculated result or calculate it.
  ///        It is guaranteed that `f` is called exactly once.
  ///        Can be called concurrently from multiple coroutines.
  /// @throws anything `f` throws.
  const T& operator()();

 private:
  std::function<T()> f_;
  std::atomic<bool> started_{false};
  utils::ResultStore<T> result_;

  engine::ConditionVariable cv_finished_;
  engine::Mutex m_finished_;
  std::atomic<bool> finished_{false};
};

template <typename T>
const T& LazyValue<T>::operator()() {
  if (finished_) return result_.Get();

  auto old = started_.exchange(true);
  if (!old) {
    try {
      result_.SetValue(f_());

      std::lock_guard lock(m_finished_);
      finished_ = true;
      cv_finished_.NotifyAll();
    } catch (...) {
      result_.SetException(std::current_exception());

      std::lock_guard lock(m_finished_);
      finished_ = true;
      cv_finished_.NotifyAll();
      throw;
    }
  } else {
    std::unique_lock lock(m_finished_);
    auto rc = cv_finished_.Wait(lock, [this]() { return finished_.load(); });
    if (!rc)
      throw engine::WaitInterruptedException(
          engine::current_task::CancellationReason());
  }

  return result_.Get();
}

}  // namespace concurrent

USERVER_NAMESPACE_END
