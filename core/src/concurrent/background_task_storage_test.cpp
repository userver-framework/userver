#include <userver/utest/utest.hpp>

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <utility>

#ifdef __linux__
#include <sched.h>
#endif

#include <engine/tests/task_processor_utils.hpp>
#include <userver/concurrent/background_task_storage.hpp>
#include <userver/concurrent/background_task_storage_fwd.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/future.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/inherited_variable.hpp>
#include <userver/utils/lazy_prvalue.hpp>
#include <utils/statistics/system_statistics.hpp>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

UTEST(BackgroundTaskStorage, Unused) {
    const concurrent::BackgroundTaskStorage bts;
    EXPECT_EQ(bts.ActiveTasksApprox(), 0);
}

UTEST(BackgroundTaskStorage, DetachAlreadyFinished) {
    concurrent::BackgroundTaskStorageCore bts;
    auto task = engine::AsyncNoTracing([] {});
    task.Wait();

    bts.Detach(std::move(task));
    EXPECT_FALSE(task.IsValid());
    EXPECT_EQ(bts.ActiveTasksApprox(), 0);
}

UTEST(BackgroundTaskStorage, TaskStart) {
    concurrent::BackgroundTaskStorage bts;

    engine::SingleConsumerEvent event;
    bts.AsyncDetach("test", [&event] { event.Send(); });

    EXPECT_TRUE(event.WaitForEvent());
}

UTEST(BackgroundTaskStorage, CriticalTaskStart) {
    concurrent::BackgroundTaskStorage bts;

    engine::SingleConsumerEvent event;
    bts.CriticalAsyncDetach("test", [&event] { event.Send(); });

    EXPECT_TRUE(event.WaitForEvent());
}

UTEST(BackgroundTaskStorage, CancelAndWaitInDtr) {
    std::atomic<bool> started{false};
    std::atomic<bool> cancelled{false};
    auto shared = std::make_shared<int>(1);
    {
        concurrent::BackgroundTaskStorage bts;

        bts.AsyncDetach("test", [shared, &started, &cancelled] {
            started = true;

            engine::SingleConsumerEvent event;
            cancelled = !event.WaitForEventFor(utest::kMaxTestWaitTime);
        });

        engine::SingleConsumerEvent event;
        EXPECT_FALSE(event.WaitForEventFor(50ms));

        EXPECT_EQ(shared.use_count(), 2);
    }

    EXPECT_TRUE(started);
    EXPECT_TRUE(cancelled);
    EXPECT_EQ(shared.use_count(), 1);
}

UTEST(BackgroundTaskStorage, Sample) {
    constexpr auto kString = "a very-very-very-very long string";
    const auto do_stuff = [] {
        // Make sure the task starts
        engine::Yield();
    };

    /// [Sample]
    std::string x = kString;
    std::string y;
    {
        // You must guarantee that 'x' and 'y' are alive during 'bts' lifetime.
        concurrent::BackgroundTaskStorage bts;

        bts.AsyncDetach("task", [&x, &y] {
            engine::InterruptibleSleepFor(20s);
            EXPECT_TRUE(engine::current_task::ShouldCancel());
            y = std::move(x);
        });

        do_stuff();

        // It is recommended to call CancelAndWait explicitly for the clarity of
        // tasks' lifetime. BTS destructor calls CancelAndWait implicitly as well.
        bts.CancelAndWait();
    }
    /// [Sample]

    EXPECT_EQ(x, "");
    EXPECT_EQ(y, kString);
}

namespace {

// Placeholders for the Doxygen snippet below; do not call Frobnicator::Launch() - UB if executed.
struct Dependencies {};
struct Foo {};
struct Bar {};

template <typename T>
void Use(T&) {}

/// [Bts field ordering]
class WellOrderedComponent {
public:
    // ...

    void Launch() {
        bts_.AsyncDetach("task", [this] {
            // OK, because foo_ will be destroyed after bts_.
            Use(foo_);

            // OK, because bar_ will be destroyed before bts_.
            Use(bar_);
        });
    }

private:
    Foo foo_;
    Bar bar_;

    // bts_ must be the last field for lifetime reasons.
    concurrent::BackgroundTaskStorage bts_;
};
/// [Bts field ordering]

[[maybe_unused]] void UseFrobnicator(WellOrderedComponent& weel_ordered) { weel_ordered.Launch(); }

/// [BtsLifetimeCapturesPitfalls]
class Frobnicator {
public:
    // ...

    void Launch(const Dependencies& stuff);

private:
    // ...
    Foo foo_;
    concurrent::BackgroundTaskStorage bts_;
    Bar bar_;
    // ...
};

void Frobnicator::Launch(const Dependencies& stuff) {
    int x{};
    bts_.AsyncDetach("task", [this, &stuff, &x] {
        // BUG! All local variables will be gone.
        // They should be captured by move or by copy.
        Use(x);

        // OK, because foo_ will be destroyed after bts_.
        Use(foo_);

        // BUG, because bar_ will be destroyed before bts_.
        Use(bar_);

        // Most likely a BUG! Unless `stuff` is contained within other fields,
        // there is probably no guarantee that it outlives `bts_`.
        // It should have been captured by move or by copy instead.
        Use(stuff);
    });
}
/// [BtsLifetimeCapturesPitfalls]

[[maybe_unused]] void UseFrobnicator(Frobnicator& frobnicator) { frobnicator.Launch({}); }

}  // namespace

UTEST(BackgroundTaskStorage, NoDeadlockWithUnstartedTasks) {
    concurrent::BackgroundTaskStorage bts;
    bts.AsyncDetach("test", [] {
        engine::SingleConsumerEvent event;

        EXPECT_FALSE(event.WaitForEvent());
    });
}

UTEST(BackgroundTaskStorage, MutableLambda) {
    concurrent::BackgroundTaskStorage bts;
    bts.AsyncDetach("test", [value = 1]() mutable {
        ++value;
        return value;
    });
}

UTEST(BackgroundTaskStorage, ActiveTasksCounter) {
    const long noop_tasks = 2;
    const long long_tasks = 3;
    concurrent::BackgroundTaskStorage bts;

    for (int i = 0; i < noop_tasks; ++i) {
        bts.AsyncDetach("noop-task", [] { /* noop */ });
    }
    for (int i = 0; i < long_tasks; ++i) {
        bts.AsyncDetach("long-task", [] {
            engine::SingleConsumerEvent event;

            EXPECT_FALSE(event.WaitForEventFor(2s));
        });
    }

    EXPECT_EQ(bts.ActiveTasksApprox(), noop_tasks + long_tasks);

    engine::SingleConsumerEvent event;
    EXPECT_FALSE(event.WaitForEventFor(50ms));

    EXPECT_EQ(bts.ActiveTasksApprox(), long_tasks);
}

UTEST(BackgroundTaskStorage, ExceptionWhilePreparingTask) {
    concurrent::BackgroundTaskStorage bts;

    const auto func = [](int x) { EXPECT_EQ(x, 42); };
    utils::LazyPrvalue x([]() -> int { throw std::runtime_error("exception during arg in-place construction"); });

    UEXPECT_THROW(bts.AsyncDetach("test", func, std::move(x)), std::runtime_error);

    // The destruction of 'bts' must not cause data races, hang or leak memory
}

UTEST(BackgroundTaskStorage, CancelAndWait) {
    std::atomic<bool> finished{false};
    concurrent::BackgroundTaskStorage bts;

    bts.AsyncDetach("", [&] {
        engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);
        finished = true;
    });

    // Make sure the task starts
    engine::Yield();

    bts.CancelAndWait();
    EXPECT_TRUE(finished);
}

UTEST(BackgroundTaskStorage, WaitAndDisposeSlow) {
    std::atomic<bool> finished{false};
    concurrent::BackgroundTaskStorage bts;
    bts.AsyncDetach("", [&] {
        engine::InterruptibleSleepFor(std::chrono::milliseconds(50));
        EXPECT_FALSE(engine::current_task::IsCancelRequested());
        finished = true;
    });

    bts.WaitAndDisposeSlow();
    EXPECT_TRUE(finished);
}

// When all three tokens share a shard, finishing the middle task must link its two neighbors to each other.
// Otherwise CancelAndWait could skip a live task or follow a dangling pointer to the middle token.
UTEST(BackgroundTaskStorage, CancelAfterMiddleTaskFinishes) {
    engine::SingleConsumerEvent finish_middle;
    concurrent::BackgroundTaskStorage bts;
    const auto wait_for_cancel = [] {
        engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);
        EXPECT_TRUE(engine::current_task::IsCancelRequested());
    };

    bts.CriticalAsyncDetach("first", wait_for_cancel);
    bts.CriticalAsyncDetach("middle", [&finish_middle] { EXPECT_TRUE(finish_middle.WaitForEvent()); });
    bts.CriticalAsyncDetach("last", wait_for_cancel);
    finish_middle.Send();

    const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);
    while (bts.ActiveTasksApprox() != 2) {
        ASSERT_FALSE(deadline.IsReached()) << "BTS did not release the middle task's token";
        engine::Yield();
    }

    bts.CancelAndWait();
}

UTEST(BackgroundTaskStorage, SleepWhileCancelled) {
    concurrent::BackgroundTaskStorageCore bts;
    engine::SingleConsumerEvent event;

    bts.Detach(utils::CriticalAsync("", [&] {
        engine::SleepFor(10ms);
        event.Send();
    }));

    bts.CancelAndWait();
    EXPECT_TRUE(event.WaitForEventFor(utest::kMaxTestWaitTime));
}

UTEST(BackgroundTaskStorage, DetachWhileWaiting) {
    std::atomic<bool> finished{false};
    concurrent::BackgroundTaskStorageCore bts;

    bts.Detach(utils::CriticalAsync("outer", [&] {
        // Make sure the main task enters CancelAndWait
        engine::Yield();

        bts.Detach(utils::CriticalAsync("inner", [&finished] {
            EXPECT_TRUE(engine::current_task::IsCancelRequested());
            finished = true;
        }));
    }));

    bts.CancelAndWait();
    EXPECT_TRUE(finished);
}

UTEST(BackgroundTaskStorage, Pimpl) {
    concurrent::BackgroundTaskStorageFastPimpl bts;
    EXPECT_EQ(bts->ActiveTasksApprox(), 0);
}

namespace {
engine::TaskInheritedVariable<int> inherited_variable;
}  // namespace

UTEST(BackgroundTaskStorage, NoTaskInheritedVariables) {
    inherited_variable.Set(42);
    EXPECT_EQ(inherited_variable.Get(), 42);

    concurrent::BackgroundTaskStorage bts;
    bts.AsyncDetach("", [] { EXPECT_FALSE(inherited_variable.GetOptional()); });
}

TEST(BackgroundTaskStorage, CustomTaskProcessor) {
    engine::tests::TwoStandaloneTaskProcessors tp;
    tp.RunBlocking([&] {
        engine::SingleConsumerEvent finished;
        concurrent::BackgroundTaskStorage bts(tp.GetSecondary());

        bts.AsyncDetach("", [&] {
            EXPECT_EQ(&engine::current_task::GetTaskProcessor(), &tp.GetSecondary());
            finished.Send();
        });

        EXPECT_TRUE(finished.WaitForEvent());
    });
}

#ifdef __linux__
// Sanitizer quarantines retain freed pages in RSS. Use live heap bytes when the runtime provides them.
// NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp,readability-identifier-naming)
extern "C" std::size_t __sanitizer_get_current_allocated_bytes() __attribute__((weak));

namespace {

struct PinCpuOptions {
    std::optional<int> excluded_cpu;
};

struct CpuSetDeleter {
    void operator()(cpu_set_t* mask) const noexcept { CPU_FREE(mask); }
};

using CpuSet = std::unique_ptr<cpu_set_t, CpuSetDeleter>;

struct CpuAffinityMask {
    CpuSet mask;
    int capacity;
};

std::optional<CpuAffinityMask> GetAllowedCpuMask() {
    for (int cpu_count = CPU_SETSIZE; cpu_count <= std::numeric_limits<int>::max() / 2; cpu_count *= 2) {
        CpuSet mask{CPU_ALLOC(cpu_count)};
        if (!mask) {
            return std::nullopt;
        }

        if (::sched_getaffinity(0, CPU_ALLOC_SIZE(cpu_count), mask.get()) == 0) {
            return CpuAffinityMask{.mask = std::move(mask), .capacity = cpu_count};
        }
        if (errno != EINVAL) {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

std::optional<int> PinToAllowedCpu(PinCpuOptions options = {}) {
    const auto allowed = GetAllowedCpuMask();
    if (!allowed) {
        return std::nullopt;
    }

    const auto size = CPU_ALLOC_SIZE(allowed->capacity);
    const CpuSet selected{CPU_ALLOC(allowed->capacity)};
    if (!selected) {
        return std::nullopt;
    }

    for (int cpu = 0; cpu < allowed->capacity; ++cpu) {
        if (cpu == options.excluded_cpu || !CPU_ISSET_S(cpu, size, allowed->mask.get())) {
            continue;
        }
        CPU_ZERO_S(size, selected.get());
        CPU_SET_S(cpu, size, selected.get());
        if (::sched_setaffinity(0, size, selected.get()) == 0) {
            return cpu;
        }
    }
    return std::nullopt;
}

std::optional<std::int64_t> GetMemoryKiB() {
    if (__sanitizer_get_current_allocated_bytes) {
        return __sanitizer_get_current_allocated_bytes() / 1024;
    }
    return utils::statistics::impl::GetSelfSystemStatistics().rss_kb;
}

}  // namespace

TEST(BackgroundTaskStorage, ReleasesMemoryAfterCrossThreadDetach) {
    // The old per-CPU free list retained 10 MiB for this many cross-CPU detaches in a local Release build.
    constexpr int kMeasuredTasks = 128 * 1024;
    constexpr std::int64_t kMaxGrowthKiB = 4 * 1024;
    engine::tests::TwoStandaloneTaskProcessors tp;
    tp.RunBlocking([&] {
        auto& worker = tp.GetSecondary();
        const auto producer_cpu = PinToAllowedCpu();
        if (!producer_cpu ||
            !engine::CriticalAsyncNoTracing(worker, PinToAllowedCpu, PinCpuOptions{.excluded_cpu = producer_cpu}).Get())
        {
            GTEST_SKIP() << "Could not pin the two task processor threads to distinct allowed CPUs";
        }

        engine::SingleConsumerEvent registered;
        concurrent::BackgroundTaskStorageCore bts;
        const auto run_tasks = [&](int count) {
            for (int i = 0; i < count; ++i) {
                engine::Promise<void> promise;
                auto finished = promise.get_future();
                bts.Detach(engine::CriticalAsyncNoTracing(
                    worker,
                    [&registered, promise = std::move(promise)]() mutable {
                        if (registered.WaitForEvent()) {
                            promise.set_value();
                        }
                    }
                ));
                // A task that finishes before Detach registers its token can return it on the producer CPU.
                registered.Send();
                finished.get();
            }
            // The single worker finishes disposing the last token before running this task.
            engine::CriticalAsyncNoTracing(worker, [] {}).Get();
        };

        run_tasks(1024);  // Warm up the coroutine stacks and allocator caches.
        const auto before = GetMemoryKiB();
        if (!before) {
            GTEST_SKIP() << "Could not read RSS from /proc/self/stat before the detach loop";
        }
        run_tasks(kMeasuredTasks);
        const auto after = GetMemoryKiB();
        if (!after) {
            GTEST_SKIP() << "Could not read RSS from /proc/self/stat after the detach loop";
        }
        const auto growth_kb = after.value() - before.value();
        bts.CancelAndWait();
        RecordProperty("memory_growth_kb", growth_kb);
        EXPECT_LT(growth_kb, kMaxGrowthKiB);
    });
}
#endif

TEST(BackgroundTaskStorage, StrongTaskProcessorBinding) {
    engine::tests::TwoStandaloneTaskProcessors tp;
    tp.RunBlocking([&] {
        engine::SingleConsumerEvent finished;
        concurrent::BackgroundTaskStorage bts;

        engine::AsyncNoTracing(tp.GetSecondary(), [&] {
            bts.AsyncDetach("", [&] {
                EXPECT_EQ(&engine::current_task::GetTaskProcessor(), &tp.GetMain());
                finished.Send();
            });
        }).Get();

        EXPECT_TRUE(finished.WaitForEvent());
    });
}

UTEST(BackgroundTaskStorage, MultipleCancelAndWait) {
    concurrent::BackgroundTaskStorage bts;
    bts.AsyncDetach("test", [] {});
    bts.WaitAndDisposeSlow();
    bts.CancelAndWait();
    bts.WaitAndDisposeSlow();
    bts.CancelAndWait();
}

USERVER_NAMESPACE_END
