#pragma once

/// @file userver/utils/async.hpp
/// @brief Utility functions to start asynchronous tasks.

#include <string>
#include <utility>

#include <userver/engine/impl/task_context_factory.hpp>
#include <userver/engine/task/shared_task_with_result.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/utils/impl/source_location.hpp>
#include <userver/utils/impl/span_wrap_call.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @ingroup userver_concurrency
///
/// @brief Starts an asynchronous task.
///
/// By default, arguments are copied or moved inside the resulting
/// `TaskWithResult`, like `std::thread` does. To pass an argument by reference,
/// wrap it in `std::ref / std::cref` or capture the arguments using a lambda.
///
/// For more documentation on launching asynchronous tasks:
///
/// @see @ref intro_tasks
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
/// For maximum customization of task parameters, see @ref utils::TaskBuilder.
///
/// @param name Name of the task to show in logs
/// @param f Function to execute asynchronously
/// @param args Arguments to pass to the function
/// @returns engine::TaskWithResult
template <typename Function, typename... Args>
[[nodiscard]] auto Async(std::string name, Function&& f, Args&&... args) {
    return engine::impl::MakeTaskWithResult<engine::TaskWithResult>(
        engine::impl::TaskConfig{},
        utils::impl::SpanLazyPrvalue(std::move(name)),
        std::forward<Function>(f),
        std::forward<Args>(args)...
    );
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
[[nodiscard]] auto Async(engine::TaskProcessor& task_processor, std::string name, Function&& f, Args&&... args) {
    return engine::impl::MakeTaskWithResult<engine::TaskWithResult>(
        engine::impl::TaskConfig{.task_processor = &task_processor},
        utils::impl::SpanLazyPrvalue(std::move(name)),
        std::forward<Function>(f),
        std::forward<Args>(args)...
    );
}

/// @overload
/// @ingroup userver_concurrency
///
/// Starts an asynchronous task with a hidden tracing::Span. The span has
/// @ref logging::Level::kNone log level, while logs within the span remain
/// linked to the nearest visible span.
///
/// Task execution may be cancelled before the function starts execution
/// in case of TaskProcessor overload.
///
/// @param f Function to execute asynchronously
/// @param args Arguments to pass to the function
/// @returns engine::TaskWithResult
template <typename Function, typename... Args>
[[nodiscard]] auto AsyncHideSpan(Function&& f, Args&&... args) {
    return engine::impl::MakeTaskWithResult<engine::TaskWithResult>(
        engine::impl::TaskConfig{},
        utils::impl::SpanLazyPrvalue(
            std::string{},
            utils::impl::SpanWrapCall::InheritVariables::kYes,
            utils::impl::SpanWrapCall::HideSpan::kYes
        ),
        std::forward<Function>(f),
        std::forward<Args>(args)...
    );
}

/// @overload
/// @ingroup userver_concurrency
///
/// Starts an asynchronous task with a hidden tracing::Span. The span has
/// @ref logging::Level::kNone log level, while logs within the span remain
/// linked to the nearest visible span.
///
/// Task execution may be cancelled before the function starts execution
/// in case of TaskProcessor overload.
///
/// @param task_processor Task processor to run on
/// @param f Function to execute asynchronously
/// @param args Arguments to pass to the function
/// @returns engine::TaskWithResult
template <typename Function, typename... Args>
[[nodiscard]] auto AsyncHideSpan(engine::TaskProcessor& task_processor, Function&& f, Args&&... args) {
    return engine::impl::MakeTaskWithResult<engine::TaskWithResult>(
        engine::impl::TaskConfig{.task_processor = &task_processor},
        utils::impl::SpanLazyPrvalue(
            std::string{},
            utils::impl::SpanWrapCall::InheritVariables::kYes,
            utils::impl::SpanWrapCall::HideSpan::kYes
        ),
        std::forward<Function>(f),
        std::forward<Args>(args)...
    );
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
[[nodiscard]] auto CriticalAsync(
    engine::TaskProcessor& task_processor,
    std::string name,
    Function&& f,
    Args&&... args
) {
    return engine::impl::MakeTaskWithResult<engine::TaskWithResult>(
        engine::impl::TaskConfig{
            .task_processor = &task_processor,
            .importance = engine::Task::Importance::kCritical,
        },
        utils::impl::SpanLazyPrvalue(std::move(name)),
        std::forward<Function>(f),
        std::forward<Args>(args)...
    );
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
[[nodiscard]] auto SharedAsync(engine::TaskProcessor& task_processor, std::string name, Function&& f, Args&&... args) {
    return engine::impl::MakeTaskWithResult<engine::SharedTaskWithResult>(
        engine::impl::TaskConfig{.task_processor = &task_processor},
        utils::impl::SpanLazyPrvalue(std::move(name)),
        std::forward<Function>(f),
        std::forward<Args>(args)...
    );
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
[[nodiscard]] auto CriticalAsync(std::string name, Function&& f, Args&&... args) {
    return engine::impl::MakeTaskWithResult<engine::TaskWithResult>(
        engine::impl::TaskConfig{.importance = engine::Task::Importance::kCritical},
        utils::impl::SpanLazyPrvalue(std::move(name)),
        std::forward<Function>(f),
        std::forward<Args>(args)...
    );
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
[[nodiscard]] auto SharedCriticalAsync(std::string name, Function&& f, Args&&... args) {
    return engine::impl::MakeTaskWithResult<engine::SharedTaskWithResult>(
        engine::impl::TaskConfig{.importance = engine::Task::Importance::kCritical},
        utils::impl::SpanLazyPrvalue(std::move(name)),
        std::forward<Function>(f),
        std::forward<Args>(args)...
    );
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
    return engine::impl::MakeTaskWithResult<engine::SharedTaskWithResult>(
        engine::impl::TaskConfig{},
        utils::impl::SpanLazyPrvalue(std::move(name)),
        std::forward<Function>(f),
        std::forward<Args>(args)...
    );
}

/// @overload
/// @ingroup userver_concurrency
///
/// Task execution may be cancelled before the function starts execution
/// in case of TaskProcessor overload.
///
/// @param name Name of the task to show in logs
/// @param deadline Deadline to set for the child task, upon reaching it the task will be cancelled
/// @param f Function to execute asynchronously
/// @param args Arguments to pass to the function
/// @returns engine::TaskWithResult
template <typename Function, typename... Args>
[[nodiscard]] auto Async(std::string name, engine::Deadline deadline, Function&& f, Args&&... args) {
    return engine::impl::MakeTaskWithResult<engine::TaskWithResult>(
        engine::impl::TaskConfig{.deadline = deadline},
        utils::impl::SpanLazyPrvalue(std::move(name)),
        std::forward<Function>(f),
        std::forward<Args>(args)...
    );
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
[[nodiscard]] auto AsyncBackground(
    std::string name,
    engine::TaskProcessor& task_processor,
    Function&& f,
    Args&&... args
) {
    return engine::impl::MakeTaskWithResult<engine::TaskWithResult>(
        engine::impl::TaskConfig{.task_processor = &task_processor},
        utils::impl::SpanLazyPrvalue(
            std::move(name),
            utils::impl::SpanWrapCall::InheritVariables::kNo,
            utils::impl::SpanWrapCall::HideSpan::kNo
        ),
        std::forward<Function>(f),
        std::forward<Args>(args)...
    );
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
    std::string name,
    engine::TaskProcessor& task_processor,
    Function&& f,
    Args&&... args
) {
    return engine::impl::MakeTaskWithResult<engine::TaskWithResult>(
        engine::impl::TaskConfig{
            .task_processor = &task_processor,
            .importance = engine::Task::Importance::kCritical,
        },
        utils::impl::SpanLazyPrvalue(
            std::move(name),
            utils::impl::SpanWrapCall::InheritVariables::kNo,
            utils::impl::SpanWrapCall::HideSpan::kNo
        ),
        std::forward<Function>(f),
        std::forward<Args>(args)...
    );
}

}  // namespace utils

USERVER_NAMESPACE_END
