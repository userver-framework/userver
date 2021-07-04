#pragma once

/// @file userver/utils/async.hpp
/// @brief Utility functions to start asynchronous tasks.

#include <userver/engine/async.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/task_inherited_data.hpp>

namespace utils {

/* A wrapper that obtains a Span from args, attaches it to current coroutine,
 * and applies a function to the rest of arguments.
 */
struct SpanWrapCall {
  template <typename Function, typename... Args>
  auto operator()(tracing::Span&& span,
                  impl::TaskInheritedDataStorage&& storage, Function&& f,
                  Args&&... args) const {
    impl::GetTaskInheritedDataStorage() = std::move(storage);
    span.AttachToCoroStack();
    return std::forward<Function>(f)(std::forward<Args>(args)...);
  }
};

/// @ingroup userver_concurrency
///
/// Starts an asynchronous task, execution of function is guaranteed to start
/// regardless of engine::TaskProcessor load limits.
///
/// Prefer using utils::Async if not sure that you need this.
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
  tracing::Span span(std::move(name));
  span.DetachFromCoroStack();
  auto storage = impl::GetTaskInheritedDataStorage();

  return engine::impl::CriticalAsync(
      task_processor, SpanWrapCall(), std::move(span), std::move(storage),
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
/// @param tasks_processor Task processor to run on
/// @param name Name of the task to show in logs
/// @param f Function to execute asynchronously
/// @param args Arguments to pass to the function
/// @returns engine::TaskWithResult
template <typename Function, typename... Args>
[[nodiscard]] auto Async(engine::TaskProcessor& task_processor,
                         std::string name, Function&& f, Args&&... args) {
  tracing::Span span(std::move(name));
  span.DetachFromCoroStack();
  auto storage = impl::GetTaskInheritedDataStorage();

  return engine::impl::Async(task_processor, SpanWrapCall(), std::move(span),
                             std::move(storage), std::forward<Function>(f),
                             std::forward<Args>(args)...);
}

/// @ingroup userver_concurrency
///
/// Starts an asynchronous task with deadline, task execution may be cancelled
/// before the function starts execution in case of TaskProcessor overload.
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
  tracing::Span span(std::move(name));
  span.DetachFromCoroStack();
  auto storage = impl::GetTaskInheritedDataStorage();

  return engine::impl::Async(task_processor, deadline, SpanWrapCall(),
                             std::move(span), std::move(storage),
                             std::forward<Function>(f),
                             std::forward<Args>(args)...);
}

/// @ingroup userver_concurrency
///
/// Starts an asynchronous task on current task processor, execution of
/// function is guaranteed to start regardless of engine::TaskProcessor load
/// limits.
///
/// Prefer using utils::Async if not sure that you need this.
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
