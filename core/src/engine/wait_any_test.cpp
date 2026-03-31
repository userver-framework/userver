#include <userver/utest/utest.hpp>

#include <array>
#include <atomic>
#include <chrono>

#include <gtest/gtest.h>

#include <userver/engine/async.hpp>
#include <userver/engine/future.hpp>
#include <userver/engine/impl/awaiter.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/engine/wait_any.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

using namespace std::chrono_literals;

namespace {

class TestAwaitable final : engine::impl::ContextAccessor {
public:
    TestAwaitable() = default;
    TestAwaitable(const TestAwaitable&) = delete;
    TestAwaitable(TestAwaitable&&) = delete;
    TestAwaitable& operator=(const TestAwaitable&) = delete;
    TestAwaitable& operator=(TestAwaitable&&) = delete;

    engine::impl::ContextAccessor* TryGetContextAccessor() noexcept { return ready_ ? nullptr : this; }

    void SetReady() {
        ready_ = true;
        if (awaiter_ != nullptr) {
            auto awaiter = std::move(awaiter_);
            engine::impl::Notify(std::move(awaiter), context_);
        }
    }

    bool IsReady() const noexcept override { return ready_; }

    void TryAppendAwaiter(boost::intrusive_ptr<engine::impl::Awaiter>& awaiter, std::uintptr_t context) override {
        if (ready_) {
            return;
        }
        UINVARIANT(awaiter_ == nullptr, "Awaiter already appended");
        awaiter_ = std::move(awaiter);
        context_ = context;
    }

    void RemoveAwaiter(engine::impl::Awaiter& awaiter, std::uintptr_t context) noexcept override {
        UINVARIANT(context_ == context, "Context does not match");

        if (awaiter_ == nullptr) {
            return;
        }
        UINVARIANT(awaiter_.get() == &awaiter, "Awaiter does not match");
        awaiter_ = nullptr;
    }

private:
    bool ready_{false};
    boost::intrusive_ptr<engine::impl::Awaiter> awaiter_;
    std::uintptr_t context_{0};
};

struct SimpleWaitAnyProxy {
    template <typename... Awaitables>
    std::optional<std::size_t> WaitAny(Awaitables&... awaitables) {
        return engine::WaitAny(awaitables...);
    }

    template <typename... Awaitables, typename Rep, typename Period>
    std::optional<std::size_t> WaitAnyFor(
        const std::chrono::duration<Rep, Period>& duration,
        Awaitables&... awaitables
    ) {
        return engine::WaitAnyFor(duration, awaitables...);
    }

    template <typename... Awaitables, typename Clock, typename Duration>
    std::optional<std::size_t> WaitAnyUntil(
        const std::chrono::time_point<Clock, Duration>& until,
        Awaitables&... awaitables
    ) {
        return engine::WaitAnyUntil(until, awaitables...);
    }

    template <typename... Awaitables>
    std::optional<std::size_t> WaitAnyUntil(engine::Deadline deadline, Awaitables&... awaitables) {
        return engine::WaitAnyUntil(deadline, awaitables...);
    }
};

struct StatefulWaitAnyProxy {
    template <typename... Awaitables>
    std::optional<std::size_t> WaitAny(Awaitables&... awaitables) {
        auto wait_any = engine::MakeWaitAny(awaitables...);
        return wait_any.Wait();
    }

    template <typename... Awaitables, typename Rep, typename Period>
    std::optional<std::size_t> WaitAnyFor(
        const std::chrono::duration<Rep, Period>& duration,
        Awaitables&... awaitables
    ) {
        auto wait_any = engine::MakeWaitAny(awaitables...);
        return wait_any.WaitFor(duration);
    }

    template <typename... Awaitables, typename Clock, typename Duration>
    std::optional<std::size_t> WaitAnyUntil(
        const std::chrono::time_point<Clock, Duration>& until,
        Awaitables&... awaitables
    ) {
        auto wait_any = engine::MakeWaitAny(awaitables...);
        return wait_any.WaitUntil(until);
    }

    template <typename... Awaitables>
    std::optional<std::size_t> WaitAnyUntil(engine::Deadline deadline, Awaitables&... awaitables) {
        auto wait_any = engine::MakeWaitAny(awaitables...);
        return wait_any.WaitUntil(deadline);
    }
};

using WaitAnyTestTypes = ::testing::Types<SimpleWaitAnyProxy, StatefulWaitAnyProxy>;

}  // namespace

template <typename ProxyType>
class WaitAny : public ::testing::Test {
protected:
    ProxyType MakeWaitAnyProxy() { return {}; }
};

TYPED_UTEST_SUITE(WaitAny, WaitAnyTestTypes);

TYPED_UTEST(WaitAny, VectorTasks) {
    static constexpr std::size_t kTaskCount = 4;
    static constexpr std::size_t kTaskOrderShift = 1;
    std::atomic<size_t> finished_counter{0};

    std::vector<engine::TaskWithResult<std::size_t>> tasks;
    tasks.reserve(kTaskCount);

    for (std::size_t i = 0; i < kTaskCount; i++) {
        tasks.push_back(engine::AsyncNoSpan([&finished_counter, i] {
            const std::size_t order = (i + kTaskCount - kTaskOrderShift) % kTaskCount;
            while (finished_counter < order) {
                engine::Yield();
            }
            return i;
        }));
    }
    std::array<bool, kTaskCount> completed{};
    completed.fill(false);
    for (std::size_t i = 0; i < kTaskCount; i++) {
        auto task_idx_opt = engine::WaitAny(tasks);
        ASSERT_TRUE(!!task_idx_opt);

        // After calling Get() the task will be ignored by WaitAny()
        auto task_res = tasks[*task_idx_opt].Get();
        EXPECT_EQ(task_res, (finished_counter + kTaskOrderShift) % kTaskCount);
        completed[task_res] = true;
        ++finished_counter;
    }
    for (std::size_t i = 0; i < kTaskCount; i++) {
        EXPECT_TRUE(completed[i]);
    }
    EXPECT_EQ(this->MakeWaitAnyProxy().WaitAny(tasks), std::nullopt);
}

TYPED_UTEST(WaitAny, Cancelled) {
    static constexpr std::size_t kTaskCount = 3;

    std::atomic<bool> started{false};
    auto task = engine::AsyncNoSpan([&started, this]() {
        std::vector<engine::TaskWithResult<void>> tasks;
        tasks.reserve(kTaskCount);
        for (size_t i = 0; i < kTaskCount; i++) {
            tasks.push_back(engine::AsyncNoSpan([] {
                for (;;) {
                    engine::Yield();
                    engine::current_task::CancellationPoint();
                }
            }));
        }

        started = true;
        auto task_idx_opt = this->MakeWaitAnyProxy().WaitAny(tasks);
        ASSERT_EQ(task_idx_opt, std::nullopt);
    });
    while (!started.load()) {
        engine::Yield();
    }

    task.SyncCancel();
}

TYPED_UTEST(WaitAny, VectorWithCancelledTask) {
    std::vector<engine::TaskWithResult<std::string>> tasks;
    tasks.push_back(engine::AsyncNoSpan([] { return std::string{"some_value"}; }));
    tasks[0].RequestCancel();

    auto task_idx_opt = this->MakeWaitAnyProxy().WaitAny(tasks);
    EXPECT_TRUE(!!task_idx_opt);
    UEXPECT_THROW(tasks[*task_idx_opt].Get(), engine::TaskCancelledException);
    EXPECT_EQ(this->MakeWaitAnyProxy().WaitAny(tasks), std::nullopt);
}

TYPED_UTEST(WaitAny, WaitAnyFor) {
    engine::TaskWithResult<void> tasks[] = {
        engine::AsyncNoSpan([] {
            for (;;) {
                engine::Yield();
                engine::current_task::CancellationPoint();
            }
        }),
        engine::AsyncNoSpan([] {}),
    };

    engine::Yield();

    auto task_idx_opt1 = this->MakeWaitAnyProxy().WaitAnyFor(utest::kMaxTestWaitTime, tasks);
    ASSERT_NE(task_idx_opt1, std::nullopt);
    ASSERT_EQ(*task_idx_opt1, 1);

    // test call WaitAny without Get for finished task
    ASSERT_EQ(this->MakeWaitAnyProxy().WaitAnyFor(utest::kMaxTestWaitTime, tasks), task_idx_opt1);

    tasks[*task_idx_opt1].Get();

    auto task_idx_opt2 = this->MakeWaitAnyProxy().WaitAnyFor(42ms, tasks);
    ASSERT_EQ(task_idx_opt2, std::nullopt);
}

TYPED_UTEST(WaitAny, WaitAnyUntil) {
    static constexpr std::size_t kTaskCount = 2;

    std::vector<engine::TaskWithResult<void>> tasks;
    tasks.reserve(kTaskCount);
    for (size_t i = 0; i < kTaskCount; i++) {
        tasks.push_back(engine::AsyncNoSpan([i] {
            if (i == 1) {
                engine::SleepFor(10ms);
                return;
            }
            for (;;) {
                engine::Yield();
                engine::current_task::CancellationPoint();
            }
        }));
    }

    engine::Yield();

    auto until = std::chrono::steady_clock::now() + utest::kMaxTestWaitTime;
    auto task_idx_opt1 = this->MakeWaitAnyProxy().WaitAnyUntil(until, tasks);
    ASSERT_NE(task_idx_opt1, std::nullopt);
    ASSERT_EQ(*task_idx_opt1, 1);
    tasks[*task_idx_opt1].Get();

    auto task_idx_opt2 = this->MakeWaitAnyProxy().WaitAnyUntil(engine::Deadline::FromDuration(42ms), tasks);
    ASSERT_EQ(task_idx_opt2, std::nullopt);
}

TYPED_UTEST(WaitAny, DistinctTypes) {
    auto task0 = engine::AsyncNoSpan([] {
        engine::SleepFor(30ms);
        return 1;
    });
    auto task1 = engine::AsyncNoSpan([] {
        engine::SleepFor(10ms);
        return std::string{"abc"};
    });

    int mask = 0;
    for (size_t i = 0; i < 2; i++) {
        auto task_idx_opt = this->MakeWaitAnyProxy().WaitAnyFor(utest::kMaxTestWaitTime, task0, task1);
        ASSERT_NE(task_idx_opt, std::nullopt);
        ASSERT_TRUE(*task_idx_opt == 0 || *task_idx_opt == 1);
        mask |= 1 << *task_idx_opt;
        if (*task_idx_opt == 0) {
            EXPECT_EQ(task0.Get(), 1);
        } else {
            EXPECT_EQ(task1.Get(), std::string{"abc"});
        }
    }
    EXPECT_EQ(mask, 3);
}

TYPED_UTEST(WaitAny, Sample) {
    /// [sample waitany]
    auto task0 = engine::AsyncNoSpan([] { return 1; });

    auto task1 = utils::Async("long_task", [] {
        engine::InterruptibleSleepFor(20s);
        return std::string{"abc"};
    });

    auto task_idx_opt = this->MakeWaitAnyProxy().WaitAny(task0, task1);
    ASSERT_TRUE(task_idx_opt);
    EXPECT_EQ(*task_idx_opt, 0);
    /// [sample waitany]
}

TYPED_UTEST(WaitAny, Throwing) {
    static constexpr std::size_t kTaskCount = 2;

    std::vector<engine::TaskWithResult<void>> tasks;
    tasks.reserve(kTaskCount);
    for (std::size_t i = 0; i < kTaskCount; i++) {
        tasks.push_back(engine::AsyncNoSpan([i] {
            if (i == 1) {
                throw std::runtime_error("test");
            }
            for (;;) {
                engine::Yield();
                engine::current_task::CancellationPoint();
            }
        }));
    }

    engine::Yield();

    auto task_idx_opt1 = this->MakeWaitAnyProxy().WaitAnyFor(utest::kMaxTestWaitTime, tasks);
    ASSERT_NE(task_idx_opt1, std::nullopt);
    ASSERT_EQ(*task_idx_opt1, 1);
    UEXPECT_THROW(tasks[*task_idx_opt1].Get(), std::runtime_error);

    auto task_idx_opt2 = this->MakeWaitAnyProxy().WaitAnyFor(42ms, tasks);
    ASSERT_EQ(task_idx_opt2, std::nullopt);
}

#ifndef NDEBUG
UTEST_DEATH(WaitAnyDeathTest, DuplicateTask) {
    static constexpr std::size_t kTaskCount = 2;

    std::vector<engine::TaskWithResult<void>> tasks;
    tasks.reserve(kTaskCount);
    for (std::size_t i = 0; i < kTaskCount; i++) {
        tasks.push_back(engine::AsyncNoSpan([] { engine::SleepFor(10ms); }));
    }

    UEXPECT_DEATH(engine::WaitAny(tasks[0], tasks[1], tasks[0]), "");
    UEXPECT_DEATH(engine::WaitAnyFor(utest::kMaxTestWaitTime, tasks[0], tasks[1], tasks[0]), "");
    UEXPECT_DEATH(engine::WaitAnyUntil(engine::Deadline::FromDuration(42ms), tasks[0], tasks[1], tasks[0]), "");
}
#endif

TYPED_UTEST(WaitAny, InvalidTask) {
    engine::TaskWithResult<void> task;
    EXPECT_EQ(this->MakeWaitAnyProxy().WaitAny(task), std::nullopt);
}

TYPED_UTEST(WaitAny, NoTasks) {
    EXPECT_EQ(this->MakeWaitAnyProxy().WaitAny(), std::nullopt);
    EXPECT_EQ(this->MakeWaitAnyProxy().WaitAnyFor(utest::kMaxTestWaitTime), std::nullopt);
    EXPECT_EQ(this->MakeWaitAnyProxy().WaitAnyUntil({}), std::nullopt);

    std::vector<engine::TaskWithResult<int>> no_tasks;
    EXPECT_EQ(this->MakeWaitAnyProxy().WaitAny(no_tasks), std::nullopt);
    EXPECT_EQ(this->MakeWaitAnyProxy().WaitAnyFor(utest::kMaxTestWaitTime, no_tasks), std::nullopt);
    EXPECT_EQ(this->MakeWaitAnyProxy().WaitAnyUntil({}, no_tasks), std::nullopt);
}

TYPED_UTEST(WaitAny, HeterogenousWait) {
    constexpr int kExpectedValue = 42;

    auto task = engine::AsyncNoSpan([expected_value = kExpectedValue] {
        engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);
        return expected_value;
    });

    engine::Promise<int> promise;
    auto future = promise.get_future();

    auto notifier_task = engine::AsyncNoSpan([&] {
        engine::SleepFor(20ms);
        promise.set_value(kExpectedValue);
    });

    UEXPECT_NO_THROW(EXPECT_EQ(this->MakeWaitAnyProxy().WaitAny(task, future), 1));

    EXPECT_TRUE(task.IsValid());
    EXPECT_FALSE(task.IsFinished());
    EXPECT_EQ(future.wait_for(0s), engine::FutureStatus::kReady);
    EXPECT_EQ(future.get(), kExpectedValue);
}

UTEST(WaitAnyContext, WaitAnyContextMoveAssignemt) {
    TestAwaitable awaitable1;
    TestAwaitable awaitable2;
    auto wait_any1 = engine::MakeWaitAny(awaitable1);
    auto wait_any2 = engine::MakeWaitAny(awaitable2);

    // Force subscription to awaitable 1.
    ASSERT_EQ(wait_any1.WaitUntil(engine::Deadline::Passed()), std::nullopt);

    // This should remove the subscription from awaitable 1.
    wait_any1 = std::move(wait_any2);
    wait_any1.Append(awaitable1);

    // Force subscriptions
    ASSERT_EQ(wait_any1.WaitUntil(engine::Deadline::Passed()), std::nullopt);

    awaitable1.SetReady();
    auto ready = wait_any1.Wait();
    ASSERT_NE(ready, std::nullopt);
    EXPECT_EQ(*ready, 1);

    awaitable2.SetReady();
    ready = wait_any1.Wait();
    ASSERT_NE(ready, std::nullopt);
    EXPECT_EQ(*ready, 0);
}

UTEST(WaitAnyContext, WaitAnyContextMoveConstruction) {
    TestAwaitable awaitable;
    auto wait_any1 = engine::MakeWaitAny(awaitable);

    // Force subscription to awaitable.
    ASSERT_EQ(wait_any1.WaitUntil(engine::Deadline::Passed()), std::nullopt);

    engine::WaitAnyContext wait_any2{std::move(wait_any1)};

    ASSERT_EQ(wait_any2.WaitUntil(engine::Deadline::Passed()), std::nullopt);

    awaitable.SetReady();
    auto ready = wait_any2.Wait();
    ASSERT_NE(ready, std::nullopt);
    EXPECT_EQ(*ready, 0);
}

UTEST(WaitAnyContext, WaitAnyContextSingleVector) {
    static constexpr std::size_t kTaskCount = 4;
    static constexpr std::size_t kTaskOrderShift = 1;
    std::atomic<size_t> finished_counter{0};

    std::vector<engine::TaskWithResult<std::size_t>> tasks;
    tasks.reserve(kTaskCount);

    for (std::size_t i = 0; i < kTaskCount; i++) {
        tasks.push_back(engine::AsyncNoSpan([&finished_counter, i] {
            const std::size_t order = (i + kTaskCount - kTaskOrderShift) % kTaskCount;
            while (finished_counter < order) {
                engine::Yield();
            }
            return i;
        }));
    }
    std::array<bool, kTaskCount> completed{};
    completed.fill(false);
    engine::WaitAnyContext wait_any;
    wait_any.Append(tasks);
    ASSERT_EQ(wait_any.GetNextIndex(), kTaskCount);

    for (std::size_t i = 0; i < kTaskCount; i++) {
        ASSERT_EQ(wait_any.GetSize(), kTaskCount - i);
        const auto task_idx_opt = wait_any.Wait();
        ASSERT_TRUE(task_idx_opt.has_value());

        const auto task_res = tasks[*task_idx_opt].Get();
        EXPECT_EQ(task_res, (finished_counter + kTaskOrderShift) % kTaskCount);
        completed[task_res] = true;
        ++finished_counter;
    }
    for (std::size_t i = 0; i < kTaskCount; i++) {
        EXPECT_TRUE(completed[i]);
    }
    ASSERT_EQ(wait_any.GetSize(), 0);
    EXPECT_EQ(wait_any.Wait(), std::nullopt);
}

UTEST(WaitAnyContext, WaitAnyContextPlainAwaitables) {
    std::vector<TestAwaitable> awaitables(3);

    auto wait_any = engine::MakeWaitAny(awaitables[0], awaitables[1], awaitables[2]);
    ASSERT_EQ(wait_any.GetNextIndex(), awaitables.size());

    EXPECT_EQ(wait_any.WaitUntil(engine::Deadline::Passed()), std::nullopt);

    for (std::size_t i = 0; i < awaitables.size(); ++i) {
        ASSERT_EQ(wait_any.GetSize(), awaitables.size() - i);
        awaitables[(i + 1) % awaitables.size()].SetReady();
        auto index = wait_any.Wait();
        ASSERT_TRUE(index.has_value());
        EXPECT_EQ(*index, (i + 1) % awaitables.size());
    }
    ASSERT_EQ(wait_any.GetSize(), 0);
    EXPECT_EQ(wait_any.Wait(), std::nullopt);
}

UTEST(WaitAnyContext, WaitAnyContextMixed) {
    TestAwaitable awaitable1;
    TestAwaitable awaitable2;
    TestAwaitable awaitable3;
    std::vector<TestAwaitable> v1(2);
    std::vector<TestAwaitable> v2(2);

    // NOLINTNEXTLINE(readability-container-data-pointer)
    std::vector<TestAwaitable*> all = {&awaitable1, &v1[0], &v1[1], &awaitable2, &awaitable3, &v2[0], &v2[1]};

    auto wait_any = engine::MakeWaitAny(awaitable1, v1, awaitable2, awaitable3, v2);
    ASSERT_EQ(wait_any.GetNextIndex(), all.size());

    EXPECT_EQ(wait_any.WaitUntil(engine::Deadline::Passed()), std::nullopt);

    for (std::size_t i = 0; i < all.size(); ++i) {
        ASSERT_EQ(wait_any.GetSize(), all.size() - i);
        all[(i + 3) % all.size()]->SetReady();
        auto index = wait_any.Wait();
        ASSERT_TRUE(index.has_value());
        EXPECT_EQ(*index, (i + 3) % all.size());
    }
    ASSERT_EQ(wait_any.GetSize(), 0);
    EXPECT_EQ(wait_any.Wait(), std::nullopt);
}

UTEST(WaitAnyContext, WaitAnyContextDynamicAppend) {
    std::vector<TestAwaitable> awaitables(10);

    engine::WaitAnyContext wait_any;
    ASSERT_EQ(wait_any.GetNextIndex(), 0);

    for (std::size_t i = 0; i < awaitables.size() / 2; ++i) {
        wait_any.Append(awaitables[i * 2]);
        wait_any.Append(awaitables[i * 2 + 1]);
        ASSERT_EQ(wait_any.GetSize(), i + 2);
        ASSERT_EQ(wait_any.GetNextIndex(), (i + 1) * 2);
        EXPECT_EQ(wait_any.WaitUntil(engine::Deadline::Passed()), std::nullopt);

        awaitables[i * 2].SetReady();
        auto index = wait_any.Wait();
        ASSERT_TRUE(index.has_value());
        EXPECT_EQ(*index, i * 2);
    }

    for (std::size_t i = 0; i < awaitables.size() / 2; ++i) {
        ASSERT_EQ(wait_any.GetSize(), awaitables.size() / 2 - i);
        EXPECT_EQ(wait_any.WaitUntil(engine::Deadline::Passed()), std::nullopt);

        awaitables[i * 2 + 1].SetReady();
        auto index = wait_any.Wait();
        ASSERT_TRUE(index.has_value());
        EXPECT_EQ(*index, i * 2 + 1);
    }
    ASSERT_EQ(wait_any.GetSize(), 0);
    EXPECT_EQ(wait_any.Wait(), std::nullopt);
}

USERVER_NAMESPACE_END
