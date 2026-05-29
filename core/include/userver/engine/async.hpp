#pragma once

/// @file userver/engine/async.hpp
/// @brief Low-level TaskWithResult creation helpers

#include <userver/engine/deadline.hpp>
#include <userver/engine/impl/task_context_factory.hpp>
#include <userver/engine/task/shared_task_with_result.hpp>
#include <userver/engine/task/task_processor_fwd.hpp>
#include <userver/engine/task/task_with_result.hpp>

USERVER_NAMESPACE_BEGIN

namespace engine {

/// @brief Runs an asynchronous function call using the specified task processor.
///
/// @warning If any logs are written in the task function (outside any manual tracing::Span scopes),
/// then those logs will have no `span_id` (yikes!).
/// If you create a span there manually, it will be disconnected from the outside trace.
/// **Prefer utils::Async by default instead.**
///
/// @warning To hide a spammy span from traces, use @ref utils::AsyncHideSpan instead.
/// Logs will then be linked to the nearest span that is written out.
/// For complex cases, use @ref engine::TaskBuilder with @ref engine::TaskBuilder::HideSpan.
///
/// @warning Some clients may call tracing::Span::CurrentSpan unconditionally, so don't be too surprised
/// if they won't work without a span scope. Do write tests.
///
/// `AsyncNoTracing` is useful for:
/// * implementing periodic tasks (like utils::PeriodicTask);
/// * worker tasks that typically run until service shutdown, and it makes no sense to have a span for the task itself;
/// * some low-level (e.g. driver or filesystem-IO) code where logs are definitely not written;
/// * breaking traces e.g. this is (rarely) wanted for some background tasks.
///
/// @see @ref utils::Async for main documentation on `Async` function family.
/// @see @ref engine::TaskBuilder for more `Async` variants.
template <typename Function, typename... Args>
[[nodiscard]] auto AsyncNoTracing(TaskProcessor& task_processor, Function&& f, Args&&... args) {
    return impl::MakeTaskWithResult<TaskWithResult>(
        impl::TaskConfig{.task_processor = &task_processor},
        std::forward<Function>(f),
        std::forward<Args>(args)...
    );
}

/// @overload
template <typename Function, typename... Args>
[[nodiscard]] auto AsyncNoTracing(Function&& f, Args&&... args) {
    return impl::MakeTaskWithResult<
        TaskWithResult>(impl::TaskConfig{}, std::forward<Function>(f), std::forward<Args>(args)...);
}

/// @overload
/// @see Task::Importance::Critical
template <typename Function, typename... Args>
[[nodiscard]] auto CriticalAsyncNoTracing(TaskProcessor& task_processor, Function&& f, Args&&... args) {
    return impl::MakeTaskWithResult<TaskWithResult>(
        impl::TaskConfig{
            .task_processor = &task_processor,
            .importance = Task::Importance::kCritical,
        },
        std::forward<Function>(f),
        std::forward<Args>(args)...
    );
}

/// @overload
/// @see Task::Importance::Critical
template <typename Function, typename... Args>
[[nodiscard]] auto CriticalAsyncNoTracing(Function&& f, Args&&... args) {
    return impl::MakeTaskWithResult<TaskWithResult>(
        impl::TaskConfig{.importance = Task::Importance::kCritical},
        std::forward<Function>(f),
        std::forward<Args>(args)...
    );
}

#ifndef ARCADIA_ROOT

/// @deprecated Use @ref engine::AsyncNoTracing instead.
template <typename Function, typename... Args>
[[nodiscard, deprecated("Use AsyncNoTracing instead")]] auto AsyncNoSpan(
    TaskProcessor& task_processor,
    Function&& f,
    Args&&... args
) {
    return AsyncNoTracing(task_processor, std::forward<Function>(f), std::forward<Args>(args)...);
}

/// @deprecated Use @ref engine::AsyncNoTracing instead.
template <typename Function, typename... Args>
[[nodiscard, deprecated("Use AsyncNoTracing instead")]] auto AsyncNoSpan(Function&& f, Args&&... args) {
    return AsyncNoTracing(std::forward<Function>(f), std::forward<Args>(args)...);
}

#endif

}  // namespace engine

USERVER_NAMESPACE_END
