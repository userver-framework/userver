#pragma once

/// @file userver/utils/task_builder.hpp
/// @brief @copybrief utils::TaskBuilder

#include <userver/compiler/impl/lifetime.hpp>
#include <userver/engine/impl/task_context_factory.hpp>
#include <userver/engine/task/shared_task_with_result.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/utils/impl/span_wrap_call.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace impl {

struct TaskBuilderOptions final {
    engine::TaskProcessor* task_processor{nullptr};
    engine::Task::Importance importance{engine::Task::Importance::kNormal};
    engine::Deadline deadline;
    bool inherit_variables{true};
};

struct TaskBuilderWithoutSelectedSpanOptions final {};

struct TaskBuilderWithSpanOptions final {
    std::string span_name;
};

struct TaskBuilderHideSpanOptions final {};

struct TaskBuilderNoSpanOptions final {};

template <typename Task>
engine::impl::TaskConfig MakeTaskConfig(const TaskBuilderOptions& options) {
    return {options.task_processor, options.importance, Task::kWaitMode, options.deadline};
}

template <typename Task, typename Function, typename... Args>
Task BuildTask(
    const TaskBuilderOptions&,
    const TaskBuilderWithoutSelectedSpanOptions&,
    const utils::impl::SourceLocation&,
    Function&&,
    Args&&...
) {
    static_assert(
        !sizeof(Task),
        "Exactly one of the following methods of TaskBuilder must be called: SpanName(), NoSpan(), HideSpan()"
    );
}

template <typename Task, typename Function, typename... Args>
Task BuildTask(
    const TaskBuilderOptions& options,
    const TaskBuilderWithSpanOptions& options_ext,
    const utils::impl::SourceLocation& source_location,
    Function&& f,
    Args&&... args
) {
    return Task{engine::impl::MakeTask(
        impl::MakeTaskConfig<Task>(options),
        utils::impl::SpanLazyPrvalue(
            std::string{options_ext.span_name},
            utils::impl::SpanWrapCall::InheritVariables{options.inherit_variables},
            utils::impl::SpanWrapCall::HideSpan::kNo,
            source_location
        ),
        std::forward<Function>(f),
        std::forward<Args>(args)...
    )};
}

template <typename Task, typename Function, typename... Args>
Task BuildTask(
    const TaskBuilderOptions& options,
    const TaskBuilderHideSpanOptions&,
    const utils::impl::SourceLocation& source_location,
    Function&& f,
    Args&&... args
) {
    return Task{engine::impl::MakeTask(
        impl::MakeTaskConfig<Task>(options),
        utils::impl::SpanLazyPrvalue(
            std::string{},
            utils::impl::SpanWrapCall::InheritVariables{options.inherit_variables},
            utils::impl::SpanWrapCall::HideSpan::kYes,
            source_location
        ),
        std::forward<Function>(f),
        std::forward<Args>(args)...
    )};
}

template <typename Task, typename Function, typename... Args>
Task BuildTask(
    const TaskBuilderOptions& options,
    const TaskBuilderNoSpanOptions&,
    const utils::impl::SourceLocation&,
    Function&& f,
    Args&&... args
) {
    // TODO support NoSpan + inherited variables.
    UINVARIANT(!options.inherit_variables, "Task-inherited variables without span are not supported at the moment");

    return Task{engine::impl::MakeTask(
        impl::MakeTaskConfig<Task>(options),
        std::forward<Function>(f),
        std::forward<Args>(args)...
    )};
}

}  // namespace impl

template <typename OptionsImpl>
class TaskBuilder;

/// @brief A @ref TaskBuilder with a set span name, see @ref TaskBuilder::SpanName.
using TaskBuilderWithSpan = TaskBuilder<impl::TaskBuilderWithSpanOptions>;

/// @brief A @ref TaskBuilder with a hidden span, see @ref TaskBuilder::HideSpan.
using TaskBuilderHideSpan = TaskBuilder<impl::TaskBuilderHideSpanOptions>;

/// @brief A @ref TaskBuilder without a span, see @ref TaskBuilder::NoSpan.
using TaskBuilderNoSpan = TaskBuilder<impl::TaskBuilderNoSpanOptions>;

/// @brief A @ref TaskBuilder for which span options have not been selected yet.
using TaskBuilderBase = TaskBuilder<impl::TaskBuilderWithoutSelectedSpanOptions>;

/// @brief Builder class for @ref engine::Task and @ref engine::TaskWithResult.
///
/// Use it if you want to build a task with complex properties or even the property values are determined at runtime.
/// If you just want to start a task, use @ref utils::Async or @ref engine::AsyncNoTracing.
///
/// To use, first default-construct as `utils::TaskBuilder{}`.
///
/// Then select span mode using one of:
/// * @ref TaskBuilder::SpanName
/// * @ref TaskBuilder::HideSpan
/// * @ref TaskBuilder::NoSpan
///
/// Then spawn a task using one of:
/// * @ref TaskBuilder::Build
/// * @ref TaskBuilder::BuildShared
///
/// Example:
/// @snippet core/src/utils/task_builder_test.cpp  snippet
///
/// @see @ref intro_tasks
template <typename OptionsImpl>
class TaskBuilder final {
public:
    TaskBuilder()
    requires std::same_as<TaskBuilder, TaskBuilderBase>
    = default;

    TaskBuilder(const TaskBuilder&) = default;
    TaskBuilder(TaskBuilder&&) = default;
    TaskBuilder& operator=(const TaskBuilder&) = default;
    TaskBuilder& operator=(TaskBuilder&&) = default;

    /// The following call to @ref Build will spawn a task with
    /// @ref tracing::Span with the passed name.
    [[nodiscard]] TaskBuilderWithSpan SpanName(std::string&& name)
    requires std::same_as<TaskBuilder, TaskBuilderBase>
    {
        return TaskBuilderWithSpan{*this, impl::TaskBuilderWithSpanOptions{.span_name = std::move(name)}};
    }

    /// The following call to @ref Build will spawn a task with @ref tracing::Span,
    /// but with @ref logging::Level::kNone log level (it hides the span log
    /// itself, but not logs within the span). Logs will then be linked
    /// to the nearest span that is written out.
    [[nodiscard]] TaskBuilderHideSpan HideSpan()
    requires std::same_as<TaskBuilder, TaskBuilderBase>
    {
        return TaskBuilderHideSpan{*this, impl::TaskBuilderHideSpanOptions{}};
    }

    /// The following call to @ref Build will spawn a task without
    /// any @ref tracing::Span.
    /// @see @ref engine::AsyncNoTracing()
    [[nodiscard]] TaskBuilderNoSpan NoSpan()
    requires std::same_as<TaskBuilder, TaskBuilderBase>
    {
        return TaskBuilderNoSpan{*this, impl::TaskBuilderNoSpanOptions{}};
    }

    /// Set "critical" flag for the new task.
    /// @see @ref engine::TaskBase::Importance
    TaskBuilder& Critical() USERVER_IMPL_LIFETIME_BOUND {
        options_.importance = engine::Task::Importance::kCritical;
        return *this;
    }

    /// The following call to @ref Build will spawn a task inside
    /// the defined task processor. If not called,
    /// @ref engine::current_task::GetTaskProcessor is used by default.
    TaskBuilder& TaskProcessor(engine::TaskProcessor& tp) USERVER_IMPL_LIFETIME_BOUND {
        options_.task_processor = &tp;
        return *this;
    }

    /// The following call to @ref Build will spawn a task that has a defined deadline.
    /// If the deadline expires, the task is cancelled.
    /// See `*Async*` function signatures for details.
    TaskBuilder& Deadline(engine::Deadline deadline) USERVER_IMPL_LIFETIME_BOUND {
        options_.deadline = deadline;
        return *this;
    }

    /// The following call to @ref Build will spawn a background task
    /// without propagating @ref engine::TaskInheritedVariable.
    /// @ref tracing::Span and @ref baggage::Baggage are inherited.
    TaskBuilder& Background() USERVER_IMPL_LIFETIME_BOUND {
        options_.inherit_variables = false;
        return *this;
    }

    /// Setup and return the task. It doesn't drop the previous settings,
    /// so it can be called multiple times.
    ///
    /// By default, arguments are copied or moved inside the resulting
    /// `TaskWithResult`, like `std::thread` does. To pass an argument by reference,
    /// wrap it in `std::ref / std::cref` or capture the arguments using a lambda.
    /// @returns engine::TaskWithResult
    template <typename Function, typename... Args>
    [[nodiscard]] auto Build(Function&& f, Args&&... args) const;

    /// Setup and return the task. It doesn't drop the previous settings,
    /// so it can be called multiple times.
    ///
    /// By default, arguments are copied or moved inside the resulting
    /// `TaskWithResult`, like `std::thread` does. To pass an argument by reference,
    /// wrap it in `std::ref / std::cref` or capture the arguments using a lambda.
    /// @returns engine::SharedTaskWithResult
    template <typename Function, typename... Args>
    [[nodiscard]] auto BuildShared(Function&& f, Args&&... args) const;

private:
    template <typename OtherOptions>
    friend class TaskBuilder;

    template <typename OtherOptions>
    TaskBuilder(const TaskBuilder<OtherOptions>& other, OptionsImpl&& options_ext)
        : options_(other.options_),
          options_ext_(std::move(options_ext))
    {}

    impl::TaskBuilderOptions options_{};
    [[no_unique_address]] OptionsImpl options_ext_{};
};

/// Ensures that `TaskBuilder{}` produces a @ref TaskBuilderBase.
TaskBuilder() -> TaskBuilder<impl::TaskBuilderWithoutSelectedSpanOptions>;

template <typename OptionsImpl>
template <typename Function, typename... Args>
[[nodiscard]] auto TaskBuilder<OptionsImpl>::Build(Function&& f, Args&&... args) const {
    using Task = engine::TaskWithResult<std::invoke_result_t<Function, Args...>>;
    return impl::BuildTask<Task>(
        options_,
        options_ext_,
        utils::impl::SourceLocation::Current(),
        std::forward<Function>(f),
        std::forward<Args>(args)...
    );
}

template <typename OptionsImpl>
template <typename Function, typename... Args>
[[nodiscard]] auto TaskBuilder<OptionsImpl>::BuildShared(Function&& f, Args&&... args) const {
    using Task = engine::SharedTaskWithResult<std::invoke_result_t<Function, Args...>>;
    return impl::BuildTask<Task>(
        options_,
        options_ext_,
        utils::impl::SourceLocation::Current(),
        std::forward<Function>(f),
        std::forward<Args>(args)...
    );
}

extern template class TaskBuilder<impl::TaskBuilderWithoutSelectedSpanOptions>;
extern template class TaskBuilder<impl::TaskBuilderWithSpanOptions>;
extern template class TaskBuilder<impl::TaskBuilderHideSpanOptions>;
extern template class TaskBuilder<impl::TaskBuilderNoSpanOptions>;

}  // namespace utils

USERVER_NAMESPACE_END
