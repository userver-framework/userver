#pragma once

/// @file userver/engine/async.hpp
/// @brief Low-level TaskWithResult creation helpers

#include <userver/engine/deadline.hpp>
#include <userver/engine/impl/task_context_factory.hpp>
#include <userver/engine/task/shared_task_with_result.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/utils/impl/wrapped_call.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

namespace impl {

template <template <typename> typename TaskType, typename Function, typename... Args>
[[nodiscard]] auto MakeTaskWithResult(
    TaskProcessor& task_processor,
    Task::Importance importance,
    Deadline deadline,
    Function&& f,
    Args&&... args
) {
    using ResultType = typename utils::impl::WrappedCallImplType<Function, Args...>::ResultType;
    constexpr auto kWaitMode = TaskType<ResultType>::kWaitMode;

    return TaskType<ResultType>{MakeTask(
        {task_processor, importance, kWaitMode, deadline}, std::forward<Function>(f), std::forward<Args>(args)...
    )};
}

}  // namespace impl

/// @brief Runs an asynchronous function call using the specified task processor.
///
/// @warning If any logs are written in the task function (outside any manual tracing::Span scopes),
/// then those logs will have no `span_id` (yikes!). **Prefer utils::Async by default instead.**
///
/// Also, some clients may call tracing::Span::CurrentSpan unconditionally, so don't be too surprised
/// if they won't work without a span scope. Do write tests.
///
/// `AsyncNoSpan` is useful for implementing periodic tasks (like utils::PeriodicTask), task pools,
/// and some low-level (e.g. driver) code where logs are definitely not written, or where `span_id` is useless.
///
/// @see utils::Async for main documentation on `Async` function family.
template <typename Function, typename... Args>
[[nodiscard]] auto AsyncNoSpan(TaskProcessor& task_processor, Function&& f, Args&&... args) {
    return impl::MakeTaskWithResult<TaskWithResult>(
        task_processor, Task::Importance::kNormal, {}, std::forward<Function>(f), std::forward<Args>(args)...
    );
}

/// @overload
template <typename Function, typename... Args>
[[nodiscard]] auto SharedAsyncNoSpan(TaskProcessor& task_processor, Function&& f, Args&&... args) {
    return impl::MakeTaskWithResult<SharedTaskWithResult>(
        task_processor, Task::Importance::kNormal, {}, std::forward<Function>(f), std::forward<Args>(args)...
    );
}

/// @overload
template <typename Function, typename... Args>
[[nodiscard]] auto AsyncNoSpan(TaskProcessor& task_processor, Deadline deadline, Function&& f, Args&&... args) {
    return impl::MakeTaskWithResult<TaskWithResult>(
        task_processor, Task::Importance::kNormal, deadline, std::forward<Function>(f), std::forward<Args>(args)...
    );
}

/// @overload
template <typename Function, typename... Args>
[[nodiscard]] auto SharedAsyncNoSpan(TaskProcessor& task_processor, Deadline deadline, Function&& f, Args&&... args) {
    return impl::MakeTaskWithResult<SharedTaskWithResult>(
        task_processor, Task::Importance::kNormal, deadline, std::forward<Function>(f), std::forward<Args>(args)...
    );
}

/// @overload
template <typename Function, typename... Args>
[[nodiscard]] auto AsyncNoSpan(Function&& f, Args&&... args) {
    return AsyncNoSpan(current_task::GetTaskProcessor(), std::forward<Function>(f), std::forward<Args>(args)...);
}

/// @overload
template <typename Function, typename... Args>
[[nodiscard]] auto SharedAsyncNoSpan(Function&& f, Args&&... args) {
    return SharedAsyncNoSpan(current_task::GetTaskProcessor(), std::forward<Function>(f), std::forward<Args>(args)...);
}

/// @overload
template <typename Function, typename... Args>
[[nodiscard]] auto AsyncNoSpan(Deadline deadline, Function&& f, Args&&... args) {
    return AsyncNoSpan(
        current_task::GetTaskProcessor(), deadline, std::forward<Function>(f), std::forward<Args>(args)...
    );
}

/// @overload
template <typename Function, typename... Args>
[[nodiscard]] auto SharedAsyncNoSpan(Deadline deadline, Function&& f, Args&&... args) {
    return SharedAsyncNoSpan(
        current_task::GetTaskProcessor(), deadline, std::forward<Function>(f), std::forward<Args>(args)...
    );
}

/// @overload
/// @see Task::Importance::Critical
template <typename Function, typename... Args>
[[nodiscard]] auto CriticalAsyncNoSpan(TaskProcessor& task_processor, Function&& f, Args&&... args) {
    return impl::MakeTaskWithResult<TaskWithResult>(
        task_processor, Task::Importance::kCritical, {}, std::forward<Function>(f), std::forward<Args>(args)...
    );
}

/// @overload
/// @see Task::Importance::Critical
template <typename Function, typename... Args>
[[nodiscard]] auto SharedCriticalAsyncNoSpan(TaskProcessor& task_processor, Function&& f, Args&&... args) {
    return impl::MakeTaskWithResult<SharedTaskWithResult>(
        task_processor, Task::Importance::kCritical, {}, std::forward<Function>(f), std::forward<Args>(args)...
    );
}

/// @overload
/// @see Task::Importance::Critical
template <typename Function, typename... Args>
[[nodiscard]] auto CriticalAsyncNoSpan(Function&& f, Args&&... args) {
    return CriticalAsyncNoSpan(
        current_task::GetTaskProcessor(), std::forward<Function>(f), std::forward<Args>(args)...
    );
}

/// @overload
/// @see Task::Importance::Critical
template <typename Function, typename... Args>
[[nodiscard]] auto SharedCriticalAsyncNoSpan(Function&& f, Args&&... args) {
    return SharedCriticalAsyncNoSpan(
        current_task::GetTaskProcessor(), std::forward<Function>(f), std::forward<Args>(args)...
    );
}

/// @overload
/// @see Task::Importance::Critical
template <typename Function, typename... Args>
[[nodiscard]] auto CriticalAsyncNoSpan(Deadline deadline, Function&& f, Args&&... args) {
    return impl::MakeTaskWithResult<TaskWithResult>(
        current_task::GetTaskProcessor(),
        Task::Importance::kCritical,
        deadline,
        std::forward<Function>(f),
        std::forward<Args>(args)...
    );
}

}  // namespace engine

USERVER_NAMESPACE_END
