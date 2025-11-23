#include <userver/utest/utest.hpp>

#include <userver/engine/sleep.hpp>
#include <userver/engine/task/inherited_variable.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/task_builder.hpp>

USERVER_NAMESPACE_BEGIN

UTEST(TaskBuilder, Smoke) {
    utils::TaskBuilder builder;
    engine::TaskWithResult<int> task = builder.NoSpan().Build([] { return 1; });
    EXPECT_EQ(task.Get(), 1);
}

UTEST(TaskBuilder, SmokeShared) {
    utils::TaskBuilder builder;
    engine::SharedTaskWithResult<int> task = builder.NoSpan().BuildShared([] { return 1; });
    EXPECT_EQ(task.Get(), 1);
}

UTEST(TaskBuilder, SpanName) {
    utils::TaskBuilder builder;
    auto task = builder.SpanName("123").Build([] { return std::string{tracing::Span::CurrentSpan().GetName()}; });
    EXPECT_EQ(task.Get(), "123");
}

UTEST(TaskBuilder, HideSpan) {
    utils::TaskBuilder builder;
    auto task = builder.HideSpan().Build([] { return tracing::Span::CurrentSpan().GetLogLevel(); });
    EXPECT_EQ(task.Get(), logging::Level::kNone);
}

UTEST(TaskBuilder, NoSpan) {
    utils::TaskBuilder builder;
    auto task = builder.NoSpan().Build([] { return tracing::Span::CurrentSpanUnchecked(); });
    EXPECT_EQ(task.Get(), nullptr);
}

UTEST(TaskBuilder, NonCritical) {
    utils::TaskBuilder builder;
    auto task = builder.NoSpan().Build([] { return engine::current_task::impl::IsCritical(); });
    EXPECT_FALSE(task.Get());
}

UTEST(TaskBuilder, Critical) {
    utils::TaskBuilder builder;
    auto task = builder.NoSpan().Critical().Build([] { return engine::current_task::impl::IsCritical(); });
    EXPECT_TRUE(task.Get());
}

UTEST(TaskBuilder, Deadline) {
    utils::TaskBuilder builder;
    auto task = builder.NoSpan().Deadline(engine::Deadline::FromDuration(std::chrono::milliseconds(100))).Build([] {
        return engine::InterruptibleSleepFor(std::chrono::seconds(5));
    });

    task.WaitFor(std::chrono::seconds(1));
    EXPECT_TRUE(task.IsFinished());
}

engine::TaskInheritedVariable<int> task_local_variable;

UTEST(TaskBuilder, Background) {
    task_local_variable.Set(1);

    utils::TaskBuilder builder;
    auto task = builder.NoSpan().Background().Build([] { return task_local_variable.GetOptional(); });

    EXPECT_EQ(task.Get(), nullptr);
}

UTEST_DEATH(TaskBuilderDeathTest, Misuse) {
    utils::TaskBuilder builder;
    EXPECT_UINVARIANT_FAILURE((void)builder.Build([] {}));
}

USERVER_NAMESPACE_END
