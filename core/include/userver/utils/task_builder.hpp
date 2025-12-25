#pragma once

/// @file userver/utils/task_builder.hpp
/// @brief @copybrief engine::TaskBuilder

#include <variant>

#include <userver/compiler/impl/lifetime.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/impl/task_context_factory.hpp>
#include <userver/engine/task/shared_task_with_result.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/utils/impl/span_wrap_call.hpp>
#include <userver/utils/overloaded.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// Builder class for engine::Task and engine::TaskWithResult.
/// Use it if you want to build a task with complex properties
/// or even the property values are determined at runtime.
/// If you just want to start a task, use @ref utils::Async
/// or @ref engine::AsyncNoSpan.
///
/// @see @ref intro_tasks
class TaskBuilder final {
public:
    TaskBuilder() = default;

    TaskBuilder(const TaskBuilder&) = default;

    TaskBuilder(TaskBuilder&&) = default;

    TaskBuilder& operator=(const TaskBuilder&) = default;

    TaskBuilder& operator=(TaskBuilder&&) = default;

    /// Set "critical" flag for the new task.
    /// @see @ref engine::TaskBase::Importance
    TaskBuilder& Critical() USERVER_IMPL_LIFETIME_BOUND;

    /// The following call to @ref Build will spawn a task with
    /// @ref tracing::Span with the passed name.
    TaskBuilder& SpanName(std::string&& name) USERVER_IMPL_LIFETIME_BOUND;

    /// The following call to @ref Build will spawn a task with @ref tracing::Span,
    /// but with @ref logging::Level::kNone log level (it hides the span log
    /// itself, but not logs within the span). Logs will then be linked
    /// to the nearest span that is written out.
    TaskBuilder& HideSpan() USERVER_IMPL_LIFETIME_BOUND;

    /// The following call to @ref Build will spawn a task without
    /// any @ref tracing::Span.
    /// @see @ref engine::AsyncNoSpan()
    TaskBuilder& NoSpan() USERVER_IMPL_LIFETIME_BOUND;

    /// The following call to @ref Build will spawn a task inside
    /// the defined task processor. If not called,
    /// @ref engine::current_task::GetTaskProcessor is used by default.
    TaskBuilder& TaskProcessor(engine::TaskProcessor& tp) USERVER_IMPL_LIFETIME_BOUND;

    /// The following call to @ref Build will spawn a task that has a defined deadline.
    /// If the deadline expires, the task is cancelled.
    /// See `*Async*` function signatures for details.
    TaskBuilder& Deadline(engine::Deadline deadline) USERVER_IMPL_LIFETIME_BOUND;

    /// The following call to @ref Build will spawn a background task
    /// without propagating @ref engine::TaskInheritedVariable.
    /// @ref tracing::Span and @ref baggage::Baggage are inherited.
    TaskBuilder& Background() USERVER_IMPL_LIFETIME_BOUND;

    /// Setup and return the task. It doesn't drop the previous settings,
    /// so it can be called multiple times.
    ///
    /// By default, arguments are copied or moved inside the resulting
    /// `TaskWithResult`, like `std::thread` does. To pass an argument by reference,
    /// wrap it in `std::ref / std::cref` or capture the arguments using a lambda.
    /// @returns engine::TaskWithResult
    template <typename Function, typename... Args>
    auto Build(Function&& f, Args&&... args);

    /// Setup and return the task. It doesn't drop the previous settings,
    /// so it can be called multiple times.
    ///
    /// By default, arguments are copied or moved inside the resulting
    /// `TaskWithResult`, like `std::thread` does. To pass an argument by reference,
    /// wrap it in `std::ref / std::cref` or capture the arguments using a lambda.
    /// @returns engine::SharedTaskWithResult
    template <typename Function, typename... Args>
    auto BuildShared(Function&& f, Args&&... args);

private:
    engine::impl::TaskConfig MakeConfig() const;

    auto MakeSpanFunctor(
        std::string name,
        utils::impl::SpanWrapCall::HideSpan hide_span,
        const utils::impl::SourceLocation& location = utils::impl::SourceLocation::Current()
    );

    template <typename Task, typename Function, typename... Args>
    Task BuildTask(Function&& f, Args&&... args);

    struct NoSpanTag {};

    struct HideSpanTag {};

    // Note: do not use impl::TaskConfig as it stores a ref to TaskProcessor, which cannot be nullptr
    engine::TaskProcessor* tp_{nullptr};
    engine::Task::Importance importance_{engine::Task::Importance::kNormal};
    engine::Task::WaitMode wait_mode_{engine::Task::WaitMode::kSingleWaiter};
    engine::Deadline deadline_;
    std::optional<std::variant<std::string, NoSpanTag, HideSpanTag>> span_;
    utils::impl::SpanWrapCall::InheritVariables inherit_variables_{utils::impl::SpanWrapCall::InheritVariables::kYes};
};

inline auto TaskBuilder::MakeSpanFunctor(
    std::string name,
    utils::impl::SpanWrapCall::HideSpan hide_span,
    const utils::impl::SourceLocation& location
) {
    return utils::impl::SpanLazyPrvalue(std::move(name), inherit_variables_, hide_span, location);
}

template <typename Function, typename... Args>
auto TaskBuilder::Build(Function&& f, Args&&... args) {
    using Task = engine::TaskWithResult<std::invoke_result_t<Function, Args...>>;
    return BuildTask<Task>(std::forward<Function>(f), std::forward<Args>(args)...);
}

template <typename Function, typename... Args>
auto TaskBuilder::BuildShared(Function&& f, Args&&... args) {
    using Task = engine::SharedTaskWithResult<std::invoke_result_t<Function, Args...>>;
    return BuildTask<Task>(std::forward<Function>(f), std::forward<Args>(args)...);
}

template <typename Task, typename Function, typename... Args>
Task TaskBuilder::BuildTask(Function&& f, Args&&... args) {
    UINVARIANT(
        span_,
        "Exactly one of the following methods of TaskBuilder must be called: SpanName(), NoSpan(), HideSpan()"
    );

    using HideSpan = utils::impl::SpanWrapCall::HideSpan;

    return utils::Visit(
        *span_,
        [&](const std::string& name) {
            return Task{engine::impl::MakeTask(
                MakeConfig(),
                MakeSpanFunctor({name}, HideSpan::kNo),
                std::forward<Function>(f),
                std::forward<Args>(args)...
            )};
        },
        [&](NoSpanTag) {
            return Task{engine::impl::MakeTask(MakeConfig(), std::forward<Function>(f), std::forward<Args>(args)...)};
        },
        [&](HideSpanTag) {
            return Task{engine::impl::MakeTask(
                MakeConfig(),
                MakeSpanFunctor({}, HideSpan::kYes),
                std::forward<Function>(f),
                std::forward<Args>(args)...
            )};
        }
    );
}

}  // namespace utils

USERVER_NAMESPACE_END
