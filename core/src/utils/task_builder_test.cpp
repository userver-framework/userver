#include <userver/engine/single_use_event.hpp>
#include <userver/engine/wait_all_checked.hpp>
#include <userver/utest/utest.hpp>

#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/inherited_variable.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/task_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

void PrepareFrobnication() {}
void Frobnicate() {}
int ExtractFrobnicationResult() { return 42; }

}  // namespace

UTEST(TaskBuilder, Sample) {
    auto& custom_task_processor = engine::current_task::GetTaskProcessor();
    /// [sample]
    auto task = utils::TaskBuilder{}.HideSpan().TaskProcessor(custom_task_processor).Build([] {
        PrepareFrobnication();
        Frobnicate();
        return ExtractFrobnicationResult();
    });
    const auto result = task.Get();
    /// [sample]
    EXPECT_EQ(result, 42);
}

UTEST(TaskBuilder, Smoke) {
    utils::TaskBuilder builder;
    engine::TaskWithResult<int> task = builder.NoSpan().Background().Build([] { return 1; });
    EXPECT_EQ(task.Get(), 1);
}

UTEST(TaskBuilder, SmokeShared) {
    utils::TaskBuilder builder;
    engine::SharedTaskWithResult<int> task = builder.NoSpan().Background().BuildShared([] { return 1; });
    EXPECT_EQ(task.Get(), 1);
}

UTEST(TaskBuilder, MultipleAwaitOnShared) {
    utils::TaskBuilder builder;
    engine::SingleUseEvent first_ready;
    engine::SingleUseEvent second_ready;

    engine::SharedTaskWithResult<int> task = builder.NoSpan().Background().BuildShared([&first_ready, &second_ready] {
        first_ready.Wait();
        second_ready.Wait();
        engine::InterruptibleSleepFor(std::chrono::milliseconds(100));
        return 1;
    });

    auto first_res = engine::AsyncNoTracing([&first_ready, &task] {
        first_ready.Send();
        EXPECT_EQ(task.Get(), 1);
    });
    auto second_res = engine::AsyncNoTracing([&second_ready, &task] {
        second_ready.Send();
        EXPECT_EQ(task.Get(), 1);
    });

    engine::WaitAllChecked(first_res, second_res);

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
    auto task = builder.NoSpan().Background().Build([] { return tracing::Span::CurrentSpanUnchecked(); });
    EXPECT_EQ(task.Get(), nullptr);
}

UTEST(TaskBuilder, NonCritical) {
    utils::TaskBuilder builder;
    auto task = builder.NoSpan().Background().Build([] { return engine::current_task::impl::IsCritical(); });
    EXPECT_FALSE(task.Get());
}

UTEST(TaskBuilder, Critical) {
    utils::TaskBuilder builder;
    auto task = builder.NoSpan().Background().Critical().Build([] { return engine::current_task::impl::IsCritical(); });
    EXPECT_TRUE(task.Get());
}

UTEST(TaskBuilder, Deadline) {
    utils::TaskBuilder builder;
    auto task =
        builder.NoSpan()
            .Background()
            .Deadline(engine::Deadline::FromDuration(std::chrono::milliseconds(100)))
            .Build([] { engine::InterruptibleSleepFor(std::chrono::seconds(5)); });

    task.WaitFor(std::chrono::seconds(1));
    EXPECT_TRUE(task.IsFinished());
}

UTEST(TaskBuilder, ForwardsMoveOnlyType) {
    bool task_executed = false;
    auto test_function = [&task_executed](std::unique_ptr<int>) { task_executed = true; };
    utils::TaskBuilder builder;

    auto ptr = std::make_unique<int>(42);
    builder.NoSpan().Background().Build(test_function, std::move(ptr)).Get();

    EXPECT_TRUE(task_executed);
}

namespace {
engine::TaskInheritedVariable<int> task_local_variable;
}  // namespace

UTEST(TaskBuilder, Background) {
    task_local_variable.Set(1);

    utils::TaskBuilder builder;
    auto task = builder.NoSpan().Background().Build([] { return task_local_variable.GetOptional(); });

    EXPECT_EQ(task.Get(), nullptr);
}

UTEST(TaskBuilder, Reuse) {
    static constexpr std::string_view span_name = "a long span name that definitely exceeds string SSO limits";
    auto builder = utils::TaskBuilder{}.SpanName(std::string{span_name});
    for (int i = 0; i < 10; ++i) {
        auto task = builder.Build([] { return std::string{tracing::Span::CurrentSpan().GetName()}; });
        EXPECT_EQ(task.Get(), span_name);
    }
}

USERVER_NAMESPACE_END
