#pragma once

/// @file userver/utils/async.hpp
/// @brief Utility functions to start asynchronous tasks.

#include <functional>
#include <string>
#include <utility>

#include <userver/engine/async.hpp>
#include <userver/utils/fast_pimpl.hpp>
#include <userver/utils/lazy_prvalue.hpp>

// TODO remove extra includes
#include <userver/logging/log.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/task_inherited_data.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace impl {

// A wrapper that obtains a Span from args, attaches it to current coroutine,
// and applies a function to the rest of arguments.
struct SpanWrapCall {
  explicit SpanWrapCall(std::string&& name);

  SpanWrapCall(const SpanWrapCall&) = delete;
  SpanWrapCall(SpanWrapCall&&) = delete;
  SpanWrapCall& operator=(const SpanWrapCall&) = delete;
  SpanWrapCall& operator=(SpanWrapCall&&) = delete;
  ~SpanWrapCall();

  template <typename Function, typename... Args>
  auto operator()(Function&& f, Args&&... args) {
    DoBeforeInvoke();
    return std::invoke(std::forward<Function>(f), std::forward<Args>(args)...);
  }

 private:
  void DoBeforeInvoke();

  struct Impl;

  static constexpr std::size_t kImplSize = 4240;
  static constexpr std::size_t kImplAlign = 8;
  utils::FastPimpl<Impl, kImplSize, kImplAlign> pimpl_;
};

// Note: 'name' must outlive the result of this function
inline auto SpanLazyPrvalue(std::string&& name) {
  return utils::LazyPrvalue([&name] { return SpanWrapCall(std::move(name)); });
}

}  // namespace impl

/// @ingroup userver_concurrency
///
/// Starts an asynchronous task, execution of function is guaranteed to start
/// regardless of engine::TaskProcessor load limits.
///
/// Prefer using utils::Async if not sure that you need this.
///
/// By default, arguments are copied or moved inside the resulting
/// `TaskWithResult`, like `std::thread` does. To pass an argument by reference,
/// wrap it in `std::ref / std::cref` or capture the arguments using a lambda.
///
/// @param tasks_processor Task processor to run on
/// @param name Name for the tracing::Span to use with this task
/// @param f Function to execute asynchronously
/// @param args Arguments to pass to the function
/// @returns engine::TaskWithResult
template <typename Function, typename... Args>
[[nodiscard]] auto CriticalAsync(engine::TaskProcessor& task_processor,
                                 std::string name, Function&& f,
                                 Args&&... args) {
  return engine::CriticalAsyncNoSpan(
      task_processor, impl::SpanLazyPrvalue(std::move(name)),
      std::forward<Function>(f), std::forward<Args>(args)...);
}

/// @ingroup userver_concurrency
///
/// Starts an asynchronous task, task execution may be cancelled before the
/// function starts execution in case of TaskProcessor overload.
///
/// Use utils::CriticalAsync if the function execution must start and you are
/// absolutely sure that you need it.
///
/// By default, arguments are copied or moved inside the resulting
/// `TaskWithResult`, like `std::thread` does. To pass an argument by reference,
/// wrap it in `std::ref / std::cref` or capture the arguments using a lambda.
///
/// @param tasks_processor Task processor to run on
/// @param name Name of the task to show in logs
/// @param f Function to execute asynchronously
/// @param args Arguments to pass to the function
/// @returns engine::TaskWithResult
template <typename Function, typename... Args>
[[nodiscard]] auto Async(engine::TaskProcessor& task_processor,
                         std::string name, Function&& f, Args&&... args) {
  return engine::AsyncNoSpan(
      task_processor, impl::SpanLazyPrvalue(std::move(name)),
      std::forward<Function>(f), std::forward<Args>(args)...);
}

/// @ingroup userver_concurrency
///
/// Starts an asynchronous task with deadline, task execution may be cancelled
/// before the function starts execution in case of TaskProcessor overload.
///
/// By default, arguments are copied or moved inside the resulting
/// `TaskWithResult`, like `std::thread` does. To pass an argument by reference,
/// wrap it in `std::ref / std::cref` or capture the arguments using a lambda.
///
/// @param tasks_processor Task processor to run on
/// @param name Name of the task to show in logs
/// @param f Function to execute asynchronously
/// @param args Arguments to pass to the function
/// @returns engine::TaskWithResult
template <typename Function, typename... Args>
[[nodiscard]] auto Async(engine::TaskProcessor& task_processor,
                         std::string name, engine::Deadline deadline,
                         Function&& f, Args&&... args) {
  return engine::AsyncNoSpan(
      task_processor, deadline, impl::SpanLazyPrvalue(std::move(name)),
      std::forward<Function>(f), std::forward<Args>(args)...);
}

/// @ingroup userver_concurrency
///
/// Starts an asynchronous task on current task processor, execution of
/// function is guaranteed to start regardless of engine::TaskProcessor load
/// limits.
///
/// Prefer using utils::Async if not sure that you need this.
///
/// By default, arguments are copied or moved inside the resulting
/// `TaskWithResult`, like `std::thread` does. To pass an argument by reference,
/// wrap it in `std::ref / std::cref` or capture the arguments using a lambda.
///
/// @param name Name for the tracing::Span to use with this task
/// @param f Function to execute asynchronously
/// @param args Arguments to pass to the function
/// @returns engine::TaskWithResult
template <typename Function, typename... Args>
[[nodiscard]] auto CriticalAsync(std::string name, Function&& f,
                                 Args&&... args) {
  return utils::CriticalAsync(engine::current_task::GetTaskProcessor(),
                              std::move(name), std::forward<Function>(f),
                              std::forward<Args>(args)...);
}

/// @ingroup userver_concurrency
///
/// Starts an asynchronous task on current task processor, task execution
/// may be cancelled before the function starts execution in case of
/// engine::TaskProcessor overload.
///
/// Use utils::CriticalAsync if the function execution must start and you are
/// absolutely sure that you need it.
///
/// By default, arguments are copied or moved inside the resulting
/// `TaskWithResult`, like `std::thread` does. To pass an argument by reference,
/// wrap it in `std::ref / std::cref` or capture the arguments using a lambda.
///
/// @param name Name of the task to show in logs
/// @param f Function to execute asynchronously
/// @param args Arguments to pass to the function
/// @returns engine::TaskWithResult
template <typename Function, typename... Args>
[[nodiscard]] auto Async(std::string name, Function&& f, Args&&... args) {
  return utils::Async(engine::current_task::GetTaskProcessor(), std::move(name),
                      std::forward<Function>(f), std::forward<Args>(args)...);
}

/// @ingroup userver_concurrency
///
/// Starts an asynchronous task with deadline on current task processor, task
/// execution may be cancelled before the function starts execution in case of
/// engine::TaskProcessor overload.
///
/// By default, arguments are copied or moved inside the resulting
/// `TaskWithResult`, like `std::thread` does. To pass an argument by reference,
/// wrap it in `std::ref / std::cref` or capture the arguments using a lambda.
///
/// @param name Name of the task to show in logs
/// @param f Function to execute asynchronously
/// @param args Arguments to pass to the function
/// @returns engine::TaskWithResult
template <typename Function, typename... Args>
[[nodiscard]] auto Async(std::string name, engine::Deadline deadline,
                         Function&& f, Args&&... args) {
  return utils::Async(engine::current_task::GetTaskProcessor(), std::move(name),
                      deadline, std::forward<Function>(f),
                      std::forward<Args>(args)...);
}

}  // namespace utils

USERVER_NAMESPACE_END
