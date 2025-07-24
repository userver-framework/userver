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

    explicit SpanWrapCall(std::string&& name, InheritVariables inherit_variables, const SourceLocation& location);

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

    static constexpr std::size_t kImplSize = 4432;
    static constexpr std::size_t kImplAlign = 8;
    utils::FastPimpl<Impl, kImplSize, kImplAlign> pimpl_;
};

// Note: 'name' and 'location' must outlive the result of this function
inline auto SpanLazyPrvalue(
    std::string&& name,
    SpanWrapCall::InheritVariables inherit_variables = SpanWrapCall::InheritVariables::kYes,
    const SourceLocation& location = SourceLocation::Current()
) {
    return utils::LazyPrvalue([&name, inherit_variables, &location] {
        return SpanWrapCall(std::move(name), inherit_variables, location);
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
/// @param name Name of the task to show in logs
/// @param f Function to execute asynchronously
/// @param args Arguments to pass to the function
/// @returns engine::TaskWithResult
template <typename Function, typename... Args>
[[nodiscard]] auto Async(std::string name, Function&& f, Args&&... args) {
    return engine::AsyncNoSpan(
        engine::current_task::GetTaskProcessor(),
        impl::SpanLazyPrvalue(std::move(name)),
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
    return engine::AsyncNoSpan(
        task_processor, impl::SpanLazyPrvalue(std::move(name)), std::forward<Function>(f), std::forward<Args>(args)...
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
[[nodiscard]] auto
CriticalAsync(engine::TaskProcessor& task_processor, std::string name, Function&& f, Args&&... args) {
    return engine::CriticalAsyncNoSpan(
        task_processor, impl::SpanLazyPrvalue(std::move(name)), std::forward<Function>(f), std::forward<Args>(args)...
    );
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
[[nodiscard]] auto
SharedCriticalAsync(engine::TaskProcessor& task_processor, std::string name, Function&& f, Args&&... args) {
    return engine::SharedCriticalAsyncNoSpan(
        task_processor, impl::SpanLazyPrvalue(std::move(name)), std::forward<Function>(f), std::forward<Args>(args)...
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
    return engine::SharedAsyncNoSpan(
        task_processor, impl::SpanLazyPrvalue(std::move(name)), std::forward<Function>(f), std::forward<Args>(args)...
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
/// @param deadline Deadline to set for the child task, upon reaching it the task will be cancelled
/// @param f Function to execute asynchronously
/// @param args Arguments to pass to the function
/// @returns engine::TaskWithResult
template <typename Function, typename... Args>
[[nodiscard]] auto Async(
    engine::TaskProcessor& task_processor,
    std::string name,
    engine::Deadline deadline,
    Function&& f,
    Args&&... args
) {
    return engine::AsyncNoSpan(
        task_processor,
        deadline,
        impl::SpanLazyPrvalue(std::move(name)),
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
/// @param deadline Deadline to set for the child task, upon reaching it the task will be cancelled
/// @param f Function to execute asynchronously
/// @param args Arguments to pass to the function
/// @returns engine::SharedTaskWithResult
template <typename Function, typename... Args>
[[nodiscard]] auto SharedAsync(
    engine::TaskProcessor& task_processor,
    std::string name,
    engine::Deadline deadline,
    Function&& f,
    Args&&... args
) {
    return engine::SharedAsyncNoSpan(
        task_processor,
        deadline,
        impl::SpanLazyPrvalue(std::move(name)),
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
    return utils::CriticalAsync(
        engine::current_task::GetTaskProcessor(),
        std::move(name),
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
    return utils::SharedCriticalAsync(
        engine::current_task::GetTaskProcessor(),
        std::move(name),
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
    return utils::SharedAsync(
        engine::current_task::GetTaskProcessor(),
        std::move(name),
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
    return utils::Async(
        engine::current_task::GetTaskProcessor(),
        std::move(name),
        deadline,
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
/// @returns engine::SharedTaskWithResult
template <typename Function, typename... Args>
[[nodiscard]] auto SharedAsync(std::string name, engine::Deadline deadline, Function&& f, Args&&... args) {
    return utils::SharedAsync(
        engine::current_task::GetTaskProcessor(),
        std::move(name),
        deadline,
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
[[nodiscard]] auto
AsyncBackground(std::string name, engine::TaskProcessor& task_processor, Function&& f, Args&&... args) {
    return engine::AsyncNoSpan(
        task_processor,
        impl::SpanLazyPrvalue(std::move(name), impl::SpanWrapCall::InheritVariables::kNo),
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
[[nodiscard]] auto
CriticalAsyncBackground(std::string name, engine::TaskProcessor& task_processor, Function&& f, Args&&... args) {
    return engine::CriticalAsyncNoSpan(
        task_processor,
        impl::SpanLazyPrvalue(std::move(name), impl::SpanWrapCall::InheritVariables::kNo),
        std::forward<Function>(f),
        std::forward<Args>(args)...
    );
}

}  // namespace utils

USERVER_NAMESPACE_END
