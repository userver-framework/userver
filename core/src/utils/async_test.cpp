#include <userver/utils/async.hpp>

#include <concepts>
#include <numeric>
#include <vector>

#include <userver/concurrent/variable.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/engine/task/inherited_variable.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utest/utest.hpp>

#include <engine/ev/thread_control.hpp>
#include <engine/task/task_context.hpp>
#include <engine/tests/task_processor_utils.hpp>

USERVER_NAMESPACE_BEGIN

UTEST(UtilsAsync, Base) {
    auto task = utils::Async("async", [] { return 1; });
    EXPECT_EQ(1, task.Get());
}

UTEST(UtilsAsync, WithDeadlineNotReached) {
    auto task = utils::Async("async", engine::Deadline::FromDuration(utest::kMaxTestWaitTime), [] { return 1; });
    EXPECT_EQ(task.Get(), 1);
}

UTEST(UtilsAsync, WithPassedDeadline) {
    auto task = utils::Async("async", engine::Deadline::FromDuration(std::chrono::seconds(-1)), [] { return 1; });
    UEXPECT_THROW(task.Get(), engine::TaskCancelledException);
}

UTEST(UtilsAsync, WithDeadlineCancellationPoint) {
    auto task = utils::Async("async", engine::Deadline::FromDuration(std::chrono::milliseconds(42)), [] {
        for (;;) {
            engine::current_task::CancellationPoint();
        }
    });
    UEXPECT_THROW(task.Get(), engine::TaskCancelledException);
}

UTEST(UtilsAsync, MemberFunctions) {
    struct NotCopyable {
        NotCopyable() = default;
        NotCopyable(const NotCopyable&) = delete;
        NotCopyable(NotCopyable&&) = default;
        // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
        int MultiplyByTwo(int x) { return x * 2; }
    };

    NotCopyable s;
    auto task = utils::Async("async", &NotCopyable::MultiplyByTwo, std::move(s), 2);
    EXPECT_EQ(task.Get(), 4);
}

// Test from https://github.com/userver-framework/userver/issues/48 by
// https://github.com/itrofimow
UTEST(UtilsAsync, FromNonWorkerThread) {
    auto& ev_thread = engine::current_task::GetEventThread();
    auto& task_processor = engine::current_task::GetTaskProcessor();
    engine::TaskWithResult<void> task;

    ev_thread.RunInEvLoopSync([&task_processor, &task] { task = utils::Async(task_processor, "just_a_task", [] {}); });

    task.Wait();
}

namespace {

using Request = int;
using Response = int;

/// [AsyncBackground component]
class AsyncRequestProcessor final {
public:
    AsyncRequestProcessor();

    void FooAsync(Request&& request);

    Response WaitAndGetAggregate();

private:
    static Response Foo(Request&& request);

    engine::TaskProcessor& task_processor_;
    concurrent::Variable<std::vector<engine::TaskWithResult<Response>>> tasks_;
};
/// [AsyncBackground component]

UTEST(UtilsAsync, AsyncBackground) {
    AsyncRequestProcessor async_request_processor;

    /// [AsyncBackground handler]
    auto handler = [&](Request&& request) {
        async_request_processor.FooAsync(std::move(request));
        return "Please wait, your request is being processed.";
    };
    /// [AsyncBackground handler]

    UEXPECT_NO_THROW(handler(2));
    UEXPECT_NO_THROW(handler(3));
    EXPECT_EQ(async_request_processor.WaitAndGetAggregate(), 10);
}

AsyncRequestProcessor::AsyncRequestProcessor()
    : task_processor_(engine::current_task::GetTaskProcessor())
{}

/// [AsyncBackground FooAsync]
void AsyncRequestProcessor::FooAsync(Request&& request) {
    auto tasks = tasks_.Lock();
    tasks->push_back(utils::AsyncBackground("foo", task_processor_, &Foo, std::move(request)));
}
/// [AsyncBackground FooAsync]

Response AsyncRequestProcessor::WaitAndGetAggregate() {
    auto tasks = tasks_.Lock();
    return std::accumulate(tasks->begin(), tasks->end(), 0, [](int sum, auto& task) { return sum + task.Get(); });
}

Response AsyncRequestProcessor::Foo(Request&& request) { return request * 2; }

}  // namespace

namespace {

engine::TaskInheritedVariable<int> kInheritedVariable;

}  // namespace

UTEST(UtilsAsync, AsyncCapturesExpectedContext) {
    kInheritedVariable.Set(42);
    const auto parent_trace_id = tracing::Span::CurrentSpan().GetTraceId();

    auto task = utils::Async("async", [&, inherited = kInheritedVariable.Get()] {
        EXPECT_TRUE(tracing::Span::CurrentSpanUnchecked());
        EXPECT_EQ(tracing::Span::CurrentSpan().GetTraceId(), parent_trace_id);
        EXPECT_EQ(inherited, 42);
        EXPECT_FALSE(engine::current_task::impl::IsCritical());
        EXPECT_FALSE(engine::current_task::IsCancelRequested());
        return true;
    });
    static_assert(std::same_as<decltype(task), engine::TaskWithResult<bool>>);

    EXPECT_TRUE(task.Get());
}

TEST(UtilsAsync, AsyncWithTaskProcessorCapturesExpectedContext) {
    engine::tests::TwoStandaloneTaskProcessors tp;
    tp.RunBlocking([&] {
        kInheritedVariable.Set(42);
        const auto parent_trace_id = tracing::Span::CurrentSpan().GetTraceId();

        auto task = utils::Async(tp.GetSecondary(), "async", [&, inherited = kInheritedVariable.Get()] {
            EXPECT_TRUE(tracing::Span::CurrentSpanUnchecked());
            EXPECT_EQ(tracing::Span::CurrentSpan().GetTraceId(), parent_trace_id);
            EXPECT_EQ(inherited, 42);
            EXPECT_FALSE(engine::current_task::impl::IsCritical());
            EXPECT_FALSE(engine::current_task::IsCancelRequested());
            EXPECT_EQ(&engine::current_task::GetTaskProcessor(), &tp.GetSecondary());
            return true;
        });
        static_assert(std::same_as<decltype(task), engine::TaskWithResult<bool>>);

        EXPECT_TRUE(task.Get());
    });
}

UTEST(UtilsAsync, AsyncHideSpanCapturesExpectedContext) {
    kInheritedVariable.Set(42);
    const auto parent_trace_id = tracing::Span::CurrentSpan().GetTraceId();

    auto task = utils::AsyncHideSpan([&, inherited = kInheritedVariable.Get()] {
        EXPECT_TRUE(tracing::Span::CurrentSpanUnchecked());
        EXPECT_EQ(tracing::Span::CurrentSpan().GetTraceId(), parent_trace_id);
        EXPECT_EQ(tracing::Span::CurrentSpan().GetLogLevel(), logging::Level::kNone);
        EXPECT_EQ(inherited, 42);
        EXPECT_FALSE(engine::current_task::impl::IsCritical());
        EXPECT_FALSE(engine::current_task::IsCancelRequested());
        return true;
    });
    static_assert(std::same_as<decltype(task), engine::TaskWithResult<bool>>);

    EXPECT_TRUE(task.Get());
}

TEST(UtilsAsync, AsyncHideSpanWithTaskProcessorCapturesExpectedContext) {
    engine::tests::TwoStandaloneTaskProcessors tp;
    tp.RunBlocking([&] {
        kInheritedVariable.Set(42);
        const auto parent_trace_id = tracing::Span::CurrentSpan().GetTraceId();

        auto task = utils::AsyncHideSpan(tp.GetSecondary(), [&, inherited = kInheritedVariable.Get()] {
            EXPECT_TRUE(tracing::Span::CurrentSpanUnchecked());
            EXPECT_EQ(tracing::Span::CurrentSpan().GetTraceId(), parent_trace_id);
            EXPECT_EQ(tracing::Span::CurrentSpan().GetLogLevel(), logging::Level::kNone);
            EXPECT_EQ(inherited, 42);
            EXPECT_FALSE(engine::current_task::impl::IsCritical());
            EXPECT_FALSE(engine::current_task::IsCancelRequested());
            EXPECT_EQ(&engine::current_task::GetTaskProcessor(), &tp.GetSecondary());
            return true;
        });
        static_assert(std::same_as<decltype(task), engine::TaskWithResult<bool>>);

        EXPECT_TRUE(task.Get());
    });
}

TEST(UtilsAsync, CriticalAsyncWithTaskProcessorCapturesExpectedContext) {
    engine::tests::TwoStandaloneTaskProcessors tp;
    tp.RunBlocking([&] {
        kInheritedVariable.Set(42);
        const auto parent_trace_id = tracing::Span::CurrentSpan().GetTraceId();

        auto task = utils::CriticalAsync(tp.GetSecondary(), "async", [&, inherited = kInheritedVariable.Get()] {
            EXPECT_TRUE(tracing::Span::CurrentSpanUnchecked());
            EXPECT_EQ(tracing::Span::CurrentSpan().GetTraceId(), parent_trace_id);
            EXPECT_EQ(inherited, 42);
            EXPECT_TRUE(engine::current_task::impl::IsCritical());
            EXPECT_FALSE(engine::current_task::IsCancelRequested());
            EXPECT_EQ(&engine::current_task::GetTaskProcessor(), &tp.GetSecondary());
            return true;
        });
        static_assert(std::same_as<decltype(task), engine::TaskWithResult<bool>>);

        EXPECT_TRUE(task.Get());
    });
}

TEST(UtilsAsync, SharedAsyncWithTaskProcessorCapturesExpectedContext) {
    engine::tests::TwoStandaloneTaskProcessors tp;
    tp.RunBlocking([&] {
        kInheritedVariable.Set(42);
        const auto parent_trace_id = tracing::Span::CurrentSpan().GetTraceId();

        auto task = utils::SharedAsync(tp.GetSecondary(), "async", [&, inherited = kInheritedVariable.Get()] {
            EXPECT_TRUE(tracing::Span::CurrentSpanUnchecked());
            EXPECT_EQ(tracing::Span::CurrentSpan().GetTraceId(), parent_trace_id);
            EXPECT_EQ(inherited, 42);
            EXPECT_FALSE(engine::current_task::impl::IsCritical());
            EXPECT_FALSE(engine::current_task::IsCancelRequested());
            EXPECT_EQ(&engine::current_task::GetTaskProcessor(), &tp.GetSecondary());
            return true;
        });
        static_assert(std::same_as<decltype(task), engine::SharedTaskWithResult<bool>>);

        EXPECT_TRUE(task.Get());
    });
}

UTEST(UtilsAsync, CriticalAsyncCapturesExpectedContext) {
    kInheritedVariable.Set(42);
    const auto parent_trace_id = tracing::Span::CurrentSpan().GetTraceId();

    auto task = utils::CriticalAsync("async", [&, inherited = kInheritedVariable.Get()] {
        EXPECT_TRUE(tracing::Span::CurrentSpanUnchecked());
        EXPECT_EQ(tracing::Span::CurrentSpan().GetTraceId(), parent_trace_id);
        EXPECT_EQ(inherited, 42);
        EXPECT_TRUE(engine::current_task::impl::IsCritical());
        EXPECT_FALSE(engine::current_task::IsCancelRequested());
        return true;
    });
    static_assert(std::same_as<decltype(task), engine::TaskWithResult<bool>>);

    EXPECT_TRUE(task.Get());
}

UTEST(UtilsAsync, SharedCriticalAsyncCapturesExpectedContext) {
    kInheritedVariable.Set(42);
    const auto parent_trace_id = tracing::Span::CurrentSpan().GetTraceId();

    auto task = utils::SharedCriticalAsync("async", [&, inherited = kInheritedVariable.Get()] {
        EXPECT_TRUE(tracing::Span::CurrentSpanUnchecked());
        EXPECT_EQ(tracing::Span::CurrentSpan().GetTraceId(), parent_trace_id);
        EXPECT_EQ(inherited, 42);
        EXPECT_TRUE(engine::current_task::impl::IsCritical());
        EXPECT_FALSE(engine::current_task::IsCancelRequested());
        return true;
    });
    static_assert(std::same_as<decltype(task), engine::SharedTaskWithResult<bool>>);

    EXPECT_TRUE(task.Get());
}

UTEST(UtilsAsync, SharedAsyncCapturesExpectedContext) {
    kInheritedVariable.Set(42);
    const auto parent_trace_id = tracing::Span::CurrentSpan().GetTraceId();

    auto task = utils::SharedAsync("async", [&, inherited = kInheritedVariable.Get()] {
        EXPECT_TRUE(tracing::Span::CurrentSpanUnchecked());
        EXPECT_EQ(tracing::Span::CurrentSpan().GetTraceId(), parent_trace_id);
        EXPECT_EQ(inherited, 42);
        EXPECT_FALSE(engine::current_task::impl::IsCritical());
        EXPECT_FALSE(engine::current_task::IsCancelRequested());
        return true;
    });
    static_assert(std::same_as<decltype(task), engine::SharedTaskWithResult<bool>>);

    EXPECT_TRUE(task.Get());
}

UTEST(UtilsAsync, AsyncWithDeadlineCapturesExpectedContext) {
    kInheritedVariable.Set(42);
    const auto parent_trace_id = tracing::Span::CurrentSpan().GetTraceId();

    const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);
    auto task = utils::Async("async", deadline, [&, inherited = kInheritedVariable.Get()] {
        EXPECT_TRUE(tracing::Span::CurrentSpanUnchecked());
        EXPECT_EQ(tracing::Span::CurrentSpan().GetTraceId(), parent_trace_id);
        EXPECT_EQ(inherited, 42);
        EXPECT_FALSE(engine::current_task::impl::IsCritical());
        EXPECT_EQ(engine::current_task::impl::GetDeadline(), deadline);
        return true;
    });
    static_assert(std::same_as<decltype(task), engine::TaskWithResult<bool>>);

    EXPECT_TRUE(task.Get());
}

TEST(UtilsAsync, AsyncBackgroundCapturesExpectedContext) {
    engine::tests::TwoStandaloneTaskProcessors tp;
    tp.RunBlocking([&] {
        kInheritedVariable.Set(42);
        const auto parent_trace_id = tracing::Span::CurrentSpan().GetTraceId();

        auto task = utils::AsyncBackground("async", tp.GetSecondary(), [&, parent_trace_id] {
            EXPECT_TRUE(tracing::Span::CurrentSpanUnchecked());
            EXPECT_EQ(tracing::Span::CurrentSpan().GetTraceId(), parent_trace_id);
            EXPECT_FALSE(kInheritedVariable.GetOptional());
            EXPECT_FALSE(engine::current_task::impl::IsCritical());
            EXPECT_FALSE(engine::current_task::IsCancelRequested());
            EXPECT_EQ(&engine::current_task::GetTaskProcessor(), &tp.GetSecondary());
            return true;
        });
        static_assert(std::same_as<decltype(task), engine::TaskWithResult<bool>>);

        EXPECT_TRUE(task.Get());
    });
}

TEST(UtilsAsync, CriticalAsyncBackgroundCapturesExpectedContext) {
    engine::tests::TwoStandaloneTaskProcessors tp;
    tp.RunBlocking([&] {
        kInheritedVariable.Set(42);
        const auto parent_trace_id = tracing::Span::CurrentSpan().GetTraceId();

        auto task = utils::CriticalAsyncBackground("async", tp.GetSecondary(), [&, parent_trace_id] {
            EXPECT_TRUE(tracing::Span::CurrentSpanUnchecked());
            EXPECT_EQ(tracing::Span::CurrentSpan().GetTraceId(), parent_trace_id);
            EXPECT_FALSE(kInheritedVariable.GetOptional());
            EXPECT_TRUE(engine::current_task::impl::IsCritical());
            EXPECT_FALSE(engine::current_task::IsCancelRequested());
            EXPECT_EQ(&engine::current_task::GetTaskProcessor(), &tp.GetSecondary());
            return true;
        });
        static_assert(std::same_as<decltype(task), engine::TaskWithResult<bool>>);

        EXPECT_TRUE(task.Get());
    });
}

USERVER_NAMESPACE_END
