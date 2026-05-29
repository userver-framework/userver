#include <userver/utest/utest.hpp>

#include <atomic>
#include <concepts>

#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/engine/task/inherited_variable.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/tracing/span.hpp>
#include <userver/utils/lazy_prvalue.hpp>
#include <userver/utils/task_builder.hpp>

#include <compiler/relax_cpu.hpp>
#include <engine/task/task_context.hpp>
#include <engine/tests/task_processor_utils.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

struct CountGuard {
    CountGuard(std::atomic<int>& count)
        : count_(count)
    {
        count_++;
    }
    CountGuard(CountGuard&& other) noexcept : count_(other.count_) {
        count_++;
    }
    CountGuard(const CountGuard& other)
        : count_(other.count_)
    {
        count_++;
    }

    ~CountGuard() { count_--; }

private:
    std::atomic<int>& count_;
};

struct OverloadedFunc final {
    struct Counters final {
        std::size_t ref_func{0};
        std::size_t cref_func{0};
        std::size_t move_func{0};

        std::size_t ref_arg{0};
        std::size_t cref_arg{0};
        std::size_t move_arg{0};
    };

    Counters& counters;

    struct Tag1 final {};
    struct Tag2 final {};
    struct Tag3 final {};

    Tag1 operator()() & { return (++counters.ref_func, Tag1{}); }
    Tag2 operator()() const& { return (++counters.cref_func, Tag2{}); }
    Tag3 operator()() && { return (++counters.move_func, Tag3{}); }

    Tag1 operator()(std::string&) { return (++counters.ref_arg, Tag1{}); }
    Tag2 operator()(const std::string&) { return (++counters.cref_arg, Tag2{}); }
    Tag3 operator()(std::string&&) { return (++counters.move_arg, Tag3{}); }
};

struct CountingConstructions {
    static inline int constructions = 0;
    static inline int destructions = 0;

    CountingConstructions() { ++constructions; }
    CountingConstructions(CountingConstructions&&) noexcept { ++constructions; }
    CountingConstructions(const CountingConstructions&) { ++constructions; }

    void operator()() const { /*noop*/ }

    CountingConstructions& operator=(CountingConstructions&&) = default;
    CountingConstructions& operator=(const CountingConstructions&) = default;

    ~CountingConstructions() { ++destructions; }
};

void ByPtrFunction() {}
void ByRefFunction() {}

const auto kDeadlineTestsTimeout = std::chrono::milliseconds(100);
const auto kMaxTestDuration = utest::kMaxTestWaitTime / 2;

}  // namespace

UTEST(Task, ArgumentsLifetime) {
    std::atomic<int> count = 0;

    EXPECT_EQ(0, count.load());
    auto task = engine::AsyncNoTracing([](CountGuard) {}, CountGuard(count));
    EXPECT_EQ(1, count.load());

    engine::Yield();
    EXPECT_EQ(0, count.load());

    task.Get();
    EXPECT_EQ(0, count.load());
}

UTEST(Task, ArgumentsLifetimeThrow) {
    std::atomic<int> count = 0;

    EXPECT_EQ(0, count.load());
    auto task = engine::AsyncNoTracing([](CountGuard) { throw std::runtime_error("123"); }, CountGuard(count));
    EXPECT_EQ(1, count.load());

    engine::Yield();
    EXPECT_EQ(0, count.load());

    UEXPECT_THROW(task.Get(), std::runtime_error);
    EXPECT_EQ(0, count.load());
}

UTEST(Task, FunctionLifetime) {
    std::atomic<int> count = 0;

    EXPECT_EQ(0, count.load());
    auto task = engine::AsyncNoTracing([guard = CountGuard(count)] {});
    EXPECT_EQ(1, count.load());

    engine::Yield();
    EXPECT_EQ(0, count.load());

    task.Get();
    EXPECT_EQ(0, count.load());
}

UTEST(Task, FunctionLifetimeThrow) {
    std::atomic<int> count = 0;

    EXPECT_EQ(0, count.load());
    auto task = engine::AsyncNoTracing([guard = CountGuard(count)] { throw std::runtime_error("123"); });
    EXPECT_EQ(1, count.load());

    engine::Yield();
    EXPECT_EQ(0, count.load());

    UEXPECT_THROW(task.Get(), std::runtime_error);
    EXPECT_EQ(0, count.load());
}

UTEST(Async, OverloadSelection) {
    OverloadedFunc::Counters counters{};
    OverloadedFunc tst{counters};
    std::string arg;

    engine::AsyncNoTracing(std::ref(tst)).Wait();
    EXPECT_EQ(counters.ref_func, 1);

    engine::AsyncNoTracing(std::cref(tst)).Wait();
    EXPECT_EQ(counters.cref_func, 1);

    engine::AsyncNoTracing(tst).Wait();
    engine::AsyncNoTracing(std::as_const(tst)).Wait();
    engine::AsyncNoTracing(std::move(tst)).Wait();
    EXPECT_EQ(counters.move_func, 3);

    engine::AsyncNoTracing(tst, std::ref(arg)).Wait();
    EXPECT_EQ(counters.ref_arg, 1);

    engine::AsyncNoTracing(tst, std::cref(arg)).Wait();
    EXPECT_EQ(counters.cref_arg, 1);

    engine::AsyncNoTracing(tst, arg).Wait();
    engine::AsyncNoTracing(tst, std::as_const(arg)).Wait();
    engine::AsyncNoTracing(tst, std::move(arg)).Wait();
    EXPECT_EQ(counters.move_arg, 3);

    UEXPECT_NO_THROW(engine::AsyncNoTracing(&ByPtrFunction).Wait());
    UEXPECT_NO_THROW(engine::AsyncNoTracing(ByRefFunction).Wait());
}

UTEST(Async, ResourceDeallocation) {
    engine::AsyncNoTracing(CountingConstructions{}).Wait();
    EXPECT_EQ(CountingConstructions::constructions, CountingConstructions::destructions);
}

UTEST(Task, CurrentTaskSetDeadline) {
    auto start = std::chrono::steady_clock::now();
    auto task = engine::AsyncNoTracing([] {
        engine::current_task::SetDeadline(engine::Deadline::FromDuration(utest::kMaxTestWaitTime));
        engine::InterruptibleSleepFor(std::chrono::milliseconds(2));
        EXPECT_FALSE(engine::current_task::IsCancelRequested());

        engine::current_task::SetDeadline(engine::Deadline::FromDuration(kDeadlineTestsTimeout));
        engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);
        EXPECT_TRUE(engine::current_task::IsCancelRequested());
    });

    UEXPECT_NO_THROW(task.Get());
    auto finish = std::chrono::steady_clock::now();
    auto duration = finish - start;
    EXPECT_GE(duration, kDeadlineTestsTimeout);
    EXPECT_LT(duration, kMaxTestDuration);
}

UTEST(Async, WithDeadline) {
    auto start = std::chrono::steady_clock::now();
    std::atomic<bool> started{false};
    const auto deadline = engine::Deadline::FromDuration(kDeadlineTestsTimeout);
    auto task = utils::TaskBuilder{}.NoSpan().Background().Deadline(deadline).Build([&started] {
        started = true;
        EXPECT_FALSE(engine::current_task::IsCancelRequested());
        engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);
        EXPECT_TRUE(engine::current_task::IsCancelRequested());
    });

    UEXPECT_NO_THROW(task.Get());
    EXPECT_TRUE(started.load());
    auto finish = std::chrono::steady_clock::now();
    auto duration = finish - start;
    EXPECT_GE(duration, kDeadlineTestsTimeout);
    EXPECT_LT(duration, kMaxTestDuration);
}

UTEST(Async, WithDeadlineDetach) {
    std::atomic<bool> started{false};
    std::atomic<bool> finished{false};
    auto task = engine::AsyncNoTracing([&started, &finished] {
        auto start = std::chrono::steady_clock::now();
        const auto deadline = engine::Deadline::FromDuration(kDeadlineTestsTimeout);
        engine::DetachUnscopedUnsafe(
            utils::TaskBuilder{}.NoSpan().Background().Deadline(deadline).Build([start, &started, &finished] {
                started = true;
                EXPECT_FALSE(engine::current_task::IsCancelRequested());
                engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);
                EXPECT_TRUE(engine::current_task::IsCancelRequested());

                auto finish = std::chrono::steady_clock::now();
                auto duration = finish - start;
                EXPECT_GE(duration, kDeadlineTestsTimeout);
                EXPECT_LT(duration, kMaxTestDuration);
                finished = true;
            })
        );
    });
    UEXPECT_NO_THROW(task.Get());
    EXPECT_TRUE(started.load());
    while (!finished) {
        engine::Yield();
    }
}

UTEST(Async, Critical) {
    auto task = engine::CriticalAsyncNoTracing([] { return true; });
    task.RequestCancel();
    task.WaitFor(std::chrono::milliseconds(100));
    EXPECT_TRUE(task.Get());
}

UTEST(Async, Emplace) {
    using namespace std::string_literals;

    auto& sync_task = engine::current_task::GetCurrentTaskContext();

    // this functor will be called synchronously
    auto append_y_factory = [&, y = "y"s] {
        EXPECT_TRUE(sync_task.IsCurrent());

        // the resulting functor will be called asynchronously
        return [&, y](auto x) {
            EXPECT_FALSE(sync_task.IsCurrent());
            return x + y;
        };
    };

    // this functor will be called synchronously, too
    auto x_factory = [&] {
        EXPECT_TRUE(sync_task.IsCurrent());
        return "x"s;
    };

    auto task = engine::CriticalAsyncNoTracing(utils::LazyPrvalue(append_y_factory), utils::LazyPrvalue(x_factory));

    EXPECT_EQ(task.Get(), "xy"s);
}

// Test from https://github.com/userver-framework/userver/issues/48 by
// https://github.com/itrofimow
UTEST(Async, FromNonWorkerThread) {
    auto& ev_thread = engine::current_task::GetEventThread();
    auto& task_processor = engine::current_task::GetTaskProcessor();
    engine::TaskWithResult<void> task;

    ev_thread.RunInEvLoopSync([&task_processor, &task] { task = engine::AsyncNoTracing(task_processor, [] {}); });

    task.Wait();
}

UTEST_MT(Async, CancelNotifyRace, 4) {
    // Stable reproduction of the race was achieved after ~10 seconds
    // (around 10'000'000 iterations) under Asan + Release + LTO.
    //
    // In CI and typical local test runs we don't have all the time in the world.
    // At least if the bug reappears, the test should fail at some point.
    const auto test_deadline = engine::Deadline::FromDuration(std::chrono::milliseconds{100});

    static constexpr auto delay = [] {
        for (int i = 0; i < 50; ++i) {
            compiler::RelaxCpu::YieldCpuDebug();
        }
    };

    while (!test_deadline.IsReached()) {
        auto task1 = engine::AsyncNoTracing([] { delay(); });

        auto task2 = engine::CriticalAsyncNoTracing([&task1] {
            try {
                task1.Wait();
            } catch (const engine::WaitInterruptedException&) {
                // There is an intentional race: if RequestCancel runs in time, Wait
                // will throw. Otherwise, it will succeed.
            }
        });

        delay();

        task2.RequestCancel();
        UEXPECT_NO_THROW(task2.Get());
        // Let's say `task2` is cancelled during `task1.Wait()`, in parallel
        // with `task1` finishing. `task1` will pull `task2` out of `WaitListLight`
        // to wake it up. At the same time, `task2.Get()` will release the task. If
        // `WaitListLight` does not have a mechanism for prolonging the waiter's
        // lifetime for such cases, `task1` will perform a use-after-free on
        // `task2`. It will be detected by Asan.
        UEXPECT_NO_THROW(task1.Get());
    }
}

namespace {

engine::TaskInheritedVariable<int> kInheritedVariable;

}  // namespace

UTEST(Async, AsyncNoTracingCapturesExpectedContext) {
    kInheritedVariable.Set(42);

    auto task = engine::AsyncNoTracing([inherited = kInheritedVariable.Get()] {
        EXPECT_FALSE(tracing::Span::CurrentSpanUnchecked());
        EXPECT_EQ(inherited, 42);
        EXPECT_FALSE(engine::current_task::impl::IsCritical());
        EXPECT_EQ(engine::current_task::impl::GetDeadline(), engine::Deadline{});
        return true;
    });
    static_assert(std::same_as<decltype(task), engine::TaskWithResult<bool>>);

    EXPECT_TRUE(task.Get());
}

TEST(Async, AsyncNoTracingWithTaskProcessorCapturesExpectedContext) {
    engine::tests::TwoStandaloneTaskProcessors tp;
    tp.RunBlocking([&] {
        kInheritedVariable.Set(42);

        auto task = engine::AsyncNoTracing(tp.GetSecondary(), [&, inherited = kInheritedVariable.Get()] {
            EXPECT_FALSE(tracing::Span::CurrentSpanUnchecked());
            EXPECT_EQ(inherited, 42);
            EXPECT_FALSE(engine::current_task::impl::IsCritical());
            EXPECT_EQ(engine::current_task::impl::GetDeadline(), engine::Deadline{});
            EXPECT_EQ(&engine::current_task::GetTaskProcessor(), &tp.GetSecondary());
            return true;
        });
        static_assert(std::same_as<decltype(task), engine::TaskWithResult<bool>>);

        EXPECT_TRUE(task.Get());
    });
}

UTEST(Async, CriticalAsyncNoTracingCapturesExpectedContext) {
    kInheritedVariable.Set(42);

    auto task = engine::CriticalAsyncNoTracing([inherited = kInheritedVariable.Get()] {
        EXPECT_FALSE(tracing::Span::CurrentSpanUnchecked());
        EXPECT_EQ(inherited, 42);
        EXPECT_TRUE(engine::current_task::impl::IsCritical());
        EXPECT_EQ(engine::current_task::impl::GetDeadline(), engine::Deadline{});
        return true;
    });
    static_assert(std::same_as<decltype(task), engine::TaskWithResult<bool>>);

    EXPECT_TRUE(task.Get());
}

TEST(Async, CriticalAsyncNoTracingWithTaskProcessorCapturesExpectedContext) {
    engine::tests::TwoStandaloneTaskProcessors tp;
    tp.RunBlocking([&] {
        kInheritedVariable.Set(42);

        auto task = engine::CriticalAsyncNoTracing(tp.GetSecondary(), [&, inherited = kInheritedVariable.Get()] {
            EXPECT_FALSE(tracing::Span::CurrentSpanUnchecked());
            EXPECT_EQ(inherited, 42);
            EXPECT_TRUE(engine::current_task::impl::IsCritical());
            EXPECT_EQ(engine::current_task::impl::GetDeadline(), engine::Deadline{});
            EXPECT_EQ(&engine::current_task::GetTaskProcessor(), &tp.GetSecondary());
            return true;
        });
        static_assert(std::same_as<decltype(task), engine::TaskWithResult<bool>>);

        EXPECT_TRUE(task.Get());
    });
}

USERVER_NAMESPACE_END
