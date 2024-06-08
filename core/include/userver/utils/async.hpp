#pragma once

/// @file userver/utils/async.hpp
/// @brief Utility functions to start asynchronous tasks.

#include <functional>
#include <string>
#include <utility>

#include <userver/engine/async.hpp>
#include <userver/utils/fast_pimpl.hpp>
#include <userver/utils/lazy_prvalue.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace impl {

// A wrapper that obtains a Span from args, attaches it to current coroutine,
// and applies a function to the rest of arguments.
struct SpanWrapCall {
  enum class InheritVariables { kYes, kNo };

  explicit SpanWrapCall(std::string&& name, InheritVariables inherit_variables);

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

  static constexpr std::size_t kImplSize = 4264;
  static constexpr std::size_t kImplAlign = 8;
  utils::FastPimpl<Impl, kImplSize, kImplAlign> pimpl_;
};

// Note: 'name' must outlive the result of this function
inline auto SpanLazyPrvalue(std::string&& name) {
  return utils::LazyPrvalue([&name] {
    return SpanWrapCall(std::move(name), SpanWrapCall::InheritVariables::kYes);
  });
}

}  // namespace impl

/// @ingroup userver_concurrency
///
/// @brief Starts an asynchronous task.
///
/// By default, arguments are copied or moved inside the resulting
/// `TaskWithResult`, like `std::thread` does. To pass an argument by reference,
/// wrap it in `std::ref / std::cref` or capture the arguments using a lambda.
///
/// @anchor flavors_of_async
/// ## Flavors of `Async`
///
/// There are multiple orthogonal parameters of the task being started.
/// Use this specific overload by default (`utils::Async`).
///
/// By engine::TaskProcessor:
///
/// * By default, task processor of the current task is used.
/// * Custom task processor can be passed as the first or second function
///   parameter (see function signatures).
///
/// By shared-ness:
///
/// * By default, functions return engine::TaskWithResult, which can be awaited
///   from 1 task at once. This is a reasonable choice for most cases.
/// * Functions from `utils::Shared*Async*` and `engine::Shared*AsyncNoSpan`
///   families return engine::SharedTaskWithResult, which can be awaited
///   from multiple tasks at the same time, at the cost of some overhead.
///
/// By engine::TaskBase::Importance ("critical-ness"):
///
/// * By default, functions can be cancelled due to engine::TaskProcessor
///   overload. Also, if the task is cancelled before being started, it will not
///   run at all.
/// * If the whole service's health (not just one request) depends on the task
///   being run, then functions from `utils::*CriticalAsync*` and
///   `engine::*CriticalAsyncNoSpan*` families can be used. There, execution of
///   the function is guaranteed to start regardless of engine::TaskProcessor
///   load limits
///
/// By tracing::Span:
///
/// * Functions from `utils::*Async*` family (which you should use by default)
///   create tracing::Span with inherited `trace_id` and `link`, a new `span_id`
///   and the specified `stopwatch_name`, which ensures that logs from the task
///   are categorized correctly and will not get lost.
/// * Functions from `engine::*AsyncNoSpan*` family create span-less tasks:
///    * A possible usage scenario is to create a task that will mostly wait
///      in the background and do various unrelated work every now and then.
///      In this case it might make sense to trace execution of work items,
///      but not of the task itself.
///    * Its usage can (albeit very rarely) be justified to squeeze some
///      nanoseconds of performance where no logging is expected.
///      But beware! Using tracing::Span::CurrentSpan() will trigger asserts
///      and lead to UB in production.
///
/// By the propagation of engine::TaskInheritedVariable instances:
///
/// * Functions from `utils::*Async*` family (which you should use by default)
///   inherit all task-inherited variables from the parent task.
/// * Functions from `engine::*AsyncNoSpan*` family do not inherit any
///   task-inherited variables.
///
/// By deadline: some `utils::*Async*` functions accept an `engine::Deadline`
/// parameter. If the deadline expires, the task is cancelled. See `*Async*`
/// function signatures for details.
///
/// ## Lifetime of captures
///
/// @note To launch background tasks, which are not awaited in the local scope,
/// use concurrent::BackgroundTaskStorage.
///
/// When launching a task, it's important to ensure that it will not access its
/// lambda captures after they are destroyed. Plain data captured by value
/// (including by move) is always safe. By-reference captures and objects that
/// store references inside are always something to be aware of. Of course,
/// copying the world will degrade performance, so let's see how to ensure
/// lifetime safety with captured references.
///
/// Task objects are automatically cancelled and awaited on destruction, if not
/// already finished. The lifetime of the task object is what determines when
/// the task may be running and accessing its captures. The task can only safely
/// capture by reference objects that outlive the task object.
///
/// When the task is just stored in a new local variable and is not moved or
/// returned from a function, capturing anything is safe:
///
/// @code
/// int x{};
/// int y{};
/// // It's recommended to write out captures explicitly when launching tasks.
/// auto task = utils::Async("frobnicate", [&x, &y] {
///   // Capturing anything defined before the `task` variable is safe.
///   Use(x, y);
/// });
/// // ...
/// task.Get();
/// @endcode
///
/// A more complicated example, where the task is moved into a container:
///
/// @code
/// // Variables are destroyed in the reverse definition order: y, tasks, x.
/// int x{};
/// std::vector<engine::TaskWithResult<void>> tasks;
/// int y{};
///
/// tasks.push_back(utils::Async("frobnicate", [&x, &y] {
///   // Capturing x is safe, because `tasks` outlives `x`.
///   Use(x);
///
///   // BUG! The task may keep running for some time after `y` is destroyed.
///   Use(y);
/// }));
/// @endcode
///
/// The bug above can be fixed by placing the declaration of `tasks` after `y`.
///
/// In the case above people often think that calling `.Get()` in appropriate
/// places solves the problem. It does not! If an exception is thrown somewhere
/// before `.Get()`, then the variables' definition order is the source
/// of truth.
///
/// Same guidelines apply when tasks are stored in classes or structs: the task
/// object must be defined below everything that it accesses:
///
/// @code
///  private:
///   Foo foo_;
///   // Can access foo_ but not bar_.
///   engine::TaskWithResult<void> task_;
///   Bar bar_;
/// @endcode
///
/// Generally, it's a good idea to put task objects as low as possible
/// in the list of class members.
///
/// Although, tasks are rarely stored in classes on practice,
/// concurrent::BackgroundTaskStorage is typically used for that purpose.
///
/// Components and their clients can always be safely captured by reference:
///
/// @see @ref scripts/docs/en/userver/component_system.md
///
/// ## About this specific overload
///
/// This is the overload that should be used by default.
///
/// * The task will be launched on the current TaskProcessor.
/// * Only 1 task may call `Wait` or `Get` on this task.
/// * The task may be cancelled before the function starts execution
///   in case of TaskProcessor overload. Also, if the task is cancelled for any
///   reason before the function starts execution, it will not run at all.
/// * The task will create a child tracing::Span with the specified name
/// * The task will inherit all engine::TaskInheritedVariable instances
///   from the current task.
///
/// For details on the various other overloads:
/// @see @ref flavors_of_async
///
/// @param name Name of the task to show in logs
/// @param f Function to execute asynchronously
/// @param args Arguments to pass to the function
/// @returns engine::TaskWithResult
template <typename Function, typename... Args>
[[nodiscard]] auto Async(std::string name, Function&& f, Args&&... args) {
  return engine::AsyncNoSpan(engine::current_task::GetTaskProcessor(),
                             impl::SpanLazyPrvalue(std::move(name)),
                             std::forward<Function>(f),
                             std::forward<Args>(args)...);
}

/// @overload
/// @ingroup userver_concurrency
///
/// Task execution may be cancelled before the function starts execution
/// in case of TaskProcessor overload.
///
/// @param task_processor Task processor to run on
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

/// @overload
/// @ingroup userver_concurrency
///
/// Execution of function is guaranteed to start regardless
/// of engine::TaskProcessor load limits. Prefer utils::Async by default.
///
/// @param task_processor Task processor to run on
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

/// @overload
/// @ingroup userver_concurrency
///
/// Execution of function is guaranteed to start regardless
/// of engine::TaskProcessor load limits. Prefer utils::SharedAsync by default.
///
/// @param task_processor Task processor to run on
/// @param name Name for the tracing::Span to use with this task
/// @param f Function to execute asynchronously
/// @param args Arguments to pass to the function
/// @returns engine::SharedTaskWithResult
template <typename Function, typename... Args>
[[nodiscard]] auto SharedCriticalAsync(engine::TaskProcessor& task_processor,
                                       std::string name, Function&& f,
                                       Args&&... args) {
  return engine::SharedCriticalAsyncNoSpan(
      task_processor, impl::SpanLazyPrvalue(std::move(name)),
      std::forward<Function>(f), std::forward<Args>(args)...);
}

/// @overload
/// @ingroup userver_concurrency
///
/// Task execution may be cancelled before the function starts execution
/// in case of TaskProcessor overload.
///
/// @param task_processor Task processor to run on
/// @param name Name of the task to show in logs
/// @param f Function to execute asynchronously
/// @param args Arguments to pass to the function
/// @returns engine::SharedTaskWithResult
template <typename Function, typename... Args>
[[nodiscard]] auto SharedAsync(engine::TaskProcessor& task_processor,
                               std::string name, Function&& f, Args&&... args) {
  return engine::SharedAsyncNoSpan(
      task_processor, impl::SpanLazyPrvalue(std::move(name)),
      std::forward<Function>(f), std::forward<Args>(args)...);
}

/// @overload
/// @ingroup userver_concurrency
///
/// Task execution may be cancelled before the function starts execution
/// in case of TaskProcessor overload.
///
/// @param task_processor Task processor to run on
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

/// @overload
/// @ingroup userver_concurrency
///
/// Task execution may be cancelled before the function starts execution
/// in case of TaskProcessor overload.
///
/// @param task_processor Task processor to run on
/// @param name Name of the task to show in logs
/// @param f Function to execute asynchronously
/// @param args Arguments to pass to the function
/// @returns engine::SharedTaskWithResult
template <typename Function, typename... Args>
[[nodiscard]] auto SharedAsync(engine::TaskProcessor& task_processor,
                               std::string name, engine::Deadline deadline,
                               Function&& f, Args&&... args) {
  return engine::SharedAsyncNoSpan(
      task_processor, deadline, impl::SpanLazyPrvalue(std::move(name)),
      std::forward<Function>(f), std::forward<Args>(args)...);
}

/// @overload
/// @ingroup userver_concurrency
///
/// Execution of function is guaranteed to start regardless
/// of engine::TaskProcessor load limits. Prefer utils::Async by default.
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

/// @overload
/// @ingroup userver_concurrency
///
/// Execution of function is guaranteed to start regardless
/// of engine::TaskProcessor load limits. Prefer utils::SharedAsync by default.
///
/// @param name Name for the tracing::Span to use with this task
/// @param f Function to execute asynchronously
/// @param args Arguments to pass to the function
/// @returns engine::SharedTaskWithResult
template <typename Function, typename... Args>
[[nodiscard]] auto SharedCriticalAsync(std::string name, Function&& f,
                                       Args&&... args) {
  return utils::SharedCriticalAsync(engine::current_task::GetTaskProcessor(),
                                    std::move(name), std::forward<Function>(f),
                                    std::forward<Args>(args)...);
}

/// @overload
/// @ingroup userver_concurrency
///
/// Task execution may be cancelled before the function starts execution
/// in case of TaskProcessor overload.
///
/// @param name Name of the task to show in logs
/// @param f Function to execute asynchronously
/// @param args Arguments to pass to the function
/// @returns engine::SharedTaskWithResult
template <typename Function, typename... Args>
[[nodiscard]] auto SharedAsync(std::string name, Function&& f, Args&&... args) {
  return utils::SharedAsync(engine::current_task::GetTaskProcessor(),
                            std::move(name), std::forward<Function>(f),
                            std::forward<Args>(args)...);
}

/// @overload
/// @ingroup userver_concurrency
///
/// Task execution may be cancelled before the function starts execution
/// in case of TaskProcessor overload.
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

/// @overload
/// @ingroup userver_concurrency
///
/// Task execution may be cancelled before the function starts execution
/// in case of TaskProcessor overload.
///
/// @param name Name of the task to show in logs
/// @param f Function to execute asynchronously
/// @param args Arguments to pass to the function
/// @returns engine::SharedTaskWithResult
template <typename Function, typename... Args>
[[nodiscard]] auto SharedAsync(std::string name, engine::Deadline deadline,
                               Function&& f, Args&&... args) {
  return utils::SharedAsync(
      engine::current_task::GetTaskProcessor(), std::move(name), deadline,
      std::forward<Function>(f), std::forward<Args>(args)...);
}

/// @ingroup userver_concurrency
///
/// Starts an asynchronous task without propagating
/// engine::TaskInheritedVariable. tracing::Span and baggage::Baggage are
/// inherited. Task execution may be cancelled before the function starts
/// execution in case of engine::TaskProcessor overload.
///
/// Typically used from a request handler to launch tasks that outlive the
/// request and do not effect its completion.
///
/// ## Usage example
/// Suppose you have some component that runs asynchronous tasks:
/// @snippet utils/async_test.cpp  AsyncBackground component
/// @snippet utils/async_test.cpp  AsyncBackground handler
///
/// If the tasks logically belong to the component itself (not to the method
/// caller), then they should be launched using utils::AsyncBackground instead
/// of the regular utils::Async
/// @snippet utils/async_test.cpp  AsyncBackground FooAsync
///
/// ## Arguments
/// By default, arguments are copied or moved inside the resulting
/// `TaskWithResult`, like `std::thread` does. To pass an argument by reference,
/// wrap it in `std::ref / std::cref` or capture the arguments using a lambda.
///
/// @param name Name of the task to show in logs
/// @param task_processor Task processor to run on
/// @param f Function to execute asynchronously
/// @param args Arguments to pass to the function
/// @returns engine::TaskWithResult
template <typename Function, typename... Args>
[[nodiscard]] auto AsyncBackground(std::string name,
                                   engine::TaskProcessor& task_processor,
                                   Function&& f, Args&&... args) {
  return engine::AsyncNoSpan(
      task_processor, utils::LazyPrvalue([&] {
        return impl::SpanWrapCall(std::move(name),
                                  impl::SpanWrapCall::InheritVariables::kNo);
      }),
      std::forward<Function>(f), std::forward<Args>(args)...);
}

/// @overload
/// @ingroup userver_concurrency
///
/// Execution of function is guaranteed to start regardless
/// of engine::TaskProcessor load limits. Use for background tasks for which
/// failing to start not just breaks handling of a single request, but harms
/// the whole service instance.
///
/// @param name Name of the task to show in logs
/// @param task_processor Task processor to run on
/// @param f Function to execute asynchronously
/// @param args Arguments to pass to the function
/// @returns engine::TaskWithResult
template <typename Function, typename... Args>
[[nodiscard]] auto CriticalAsyncBackground(
    std::string name, engine::TaskProcessor& task_processor, Function&& f,
    Args&&... args) {
  return engine::CriticalAsyncNoSpan(
      task_processor, utils::LazyPrvalue([&] {
        return impl::SpanWrapCall(std::move(name),
                                  impl::SpanWrapCall::InheritVariables::kNo);
      }),
      std::forward<Function>(f), std::forward<Args>(args)...);
}

}  // namespace utils

USERVER_NAMESPACE_END
