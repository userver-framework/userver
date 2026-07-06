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
#include <userver/utils/expected.hpp>
#include <userver/utils/slot_map.hpp>

USERVER_NAMESPACE_BEGIN

using namespace std::chrono_literals;

namespace {

class TestAwaitable final : engine::impl::AwaitableBase {
public:
    TestAwaitable() = default;
    TestAwaitable(const TestAwaitable&) = delete;
    TestAwaitable(TestAwaitable&&) = delete;
    TestAwaitable& operator=(const TestAwaitable&) = delete;
    TestAwaitable& operator=(TestAwaitable&&) = delete;

    engine::AwaitableToken GetAwaitableToken() noexcept USERVER_IMPL_LIFETIME_BOUND {
        if (ready_) {
            return {};
        }

        return engine::AwaitableToken{utils::impl::InternalTag{}, this};
    }

    void SetReady() {
        ready_ = true;
        if (awaiter_ != nullptr) {
            auto awaiter = std::move(awaiter_);
            engine::impl::NotifyAndDispose(std::move(awaiter), context_);
        }
    }

    bool IsReady() const noexcept override { return ready_; }

    void TryAppendAwaiter(engine::impl::AwaiterPtr& awaiter, std::uintptr_t context) override {
        if (ready_) {
            return;
        }
        UINVARIANT(awaiter_ == nullptr, "Awaiter already appended");
        awaiter_ = std::move(awaiter);
        context_ = context;
    }

    engine::impl::AwaiterPtr RemoveAwaiter(engine::impl::Awaiter& awaiter, std::uintptr_t context) noexcept override {
        UINVARIANT(context_ == context, "Context does not match");

        if (awaiter_ == nullptr) {
            return {};
        }
        UINVARIANT(awaiter_.get() == &awaiter, "Awaiter does not match");
        return std::move(awaiter_);
    }

private:
    bool ready_{false};
    engine::impl::AwaiterPtr awaiter_;
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
        const auto result = wait_any.Wait();
        return result.has_value() ? std::optional<std::size_t>{*result} : std::nullopt;
    }

    template <typename... Awaitables, typename Rep, typename Period>
    std::optional<std::size_t> WaitAnyFor(
        const std::chrono::duration<Rep, Period>& duration,
        Awaitables&... awaitables
    ) {
        auto wait_any = engine::MakeWaitAny(awaitables...);
        const auto result = wait_any.WaitFor(duration);
        return result.has_value() ? std::optional<std::size_t>{*result} : std::nullopt;
    }

    template <typename... Awaitables, typename Clock, typename Duration>
    std::optional<std::size_t> WaitAnyUntil(
        const std::chrono::time_point<Clock, Duration>& until,
        Awaitables&... awaitables
    ) {
        auto wait_any = engine::MakeWaitAny(awaitables...);
        const auto result = wait_any.WaitUntil(until);
        return result.has_value() ? std::optional<std::size_t>{*result} : std::nullopt;
    }

    template <typename... Awaitables>
    std::optional<std::size_t> WaitAnyUntil(engine::Deadline deadline, Awaitables&... awaitables) {
        auto wait_any = engine::MakeWaitAny(awaitables...);
        const auto result = wait_any.WaitUntil(deadline);
        return result.has_value() ? std::optional<std::size_t>{*result} : std::nullopt;
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
        tasks.push_back(engine::AsyncNoTracing([&finished_counter, i] {
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
    auto task = engine::AsyncNoTracing([&started, this]() {
        std::vector<engine::TaskWithResult<void>> tasks;
        tasks.reserve(kTaskCount);
        for (size_t i = 0; i < kTaskCount; i++) {
            tasks.push_back(engine::AsyncNoTracing([] {
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
    tasks.push_back(engine::AsyncNoTracing([] { return std::string{"some_value"}; }));
    tasks[0].RequestCancel();

    auto task_idx_opt = this->MakeWaitAnyProxy().WaitAny(tasks);
    EXPECT_TRUE(!!task_idx_opt);
    UEXPECT_THROW(tasks[*task_idx_opt].Get(), engine::TaskCancelledException);
    EXPECT_EQ(this->MakeWaitAnyProxy().WaitAny(tasks), std::nullopt);
}

TYPED_UTEST(WaitAny, WaitAnyFor) {
    engine::TaskWithResult<void> tasks[] = {
        engine::AsyncNoTracing([] {
            for (;;) {
                engine::Yield();
                engine::current_task::CancellationPoint();
            }
        }),
        engine::AsyncNoTracing([] {}),
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
        tasks.push_back(engine::AsyncNoTracing([i] {
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
    auto task0 = engine::AsyncNoTracing([] {
        engine::SleepFor(30ms);
        return 1;
    });
    auto task1 = engine::AsyncNoTracing([] {
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
    auto task0 = engine::AsyncNoTracing([] { return 1; });

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
        tasks.push_back(engine::AsyncNoTracing([i] {
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
        tasks.push_back(engine::AsyncNoTracing([] { engine::SleepFor(10ms); }));
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

    auto task = engine::AsyncNoTracing([expected_value = kExpectedValue] {
        engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);
        return expected_value;
    });

    engine::Promise<int> promise;
    auto future = promise.get_future();

    auto notifier_task = engine::AsyncNoTracing([&] {
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
    EXPECT_EQ(wait_any1.WaitUntil(engine::Deadline::Passed()), utils::unexpected(engine::WaitAnyError::kTimeout));

    // This should remove the subscription from awaitable 1.
    wait_any1 = std::move(wait_any2);
    wait_any1.Append(awaitable1);

    // Force subscriptions
    EXPECT_EQ(wait_any1.WaitUntil(engine::Deadline::Passed()), utils::unexpected(engine::WaitAnyError::kTimeout));

    awaitable1.SetReady();
    auto ready = wait_any1.Wait();
    ASSERT_TRUE(ready.has_value());
    EXPECT_EQ(*ready, 1);

    awaitable2.SetReady();
    ready = wait_any1.Wait();
    ASSERT_TRUE(ready.has_value());
    EXPECT_EQ(*ready, 0);
}

UTEST(WaitAnyContext, WaitAnyContextMoveConstruction) {
    TestAwaitable awaitable;
    auto wait_any1 = engine::MakeWaitAny(awaitable);

    // Force subscription to awaitable.
    EXPECT_EQ(wait_any1.WaitUntil(engine::Deadline::Passed()), utils::unexpected(engine::WaitAnyError::kTimeout));

    engine::WaitAnyContext wait_any2{std::move(wait_any1)};

    EXPECT_EQ(wait_any2.WaitUntil(engine::Deadline::Passed()), utils::unexpected(engine::WaitAnyError::kTimeout));

    awaitable.SetReady();
    auto ready = wait_any2.Wait();
    ASSERT_TRUE(ready.has_value());
    EXPECT_EQ(*ready, 0);
}

UTEST(WaitAnyContext, WaitAnyContextSingleVector) {
    static constexpr std::size_t kTaskCount = 4;
    static constexpr std::size_t kTaskOrderShift = 1;
    std::atomic<size_t> finished_counter{0};

    std::vector<engine::TaskWithResult<std::size_t>> tasks;
    tasks.reserve(kTaskCount);

    for (std::size_t i = 0; i < kTaskCount; i++) {
        tasks.push_back(engine::AsyncNoTracing([&finished_counter, i] {
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
    for (auto& task : tasks) {
        wait_any.Append(task);
    }
    ASSERT_EQ(wait_any.GetNextId(), kTaskCount);

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
    EXPECT_EQ(wait_any.Wait(), utils::unexpected(engine::WaitAnyError::kEmpty));
}

UTEST(WaitAnyContext, WaitAnyContextPlainAwaitables) {
    /// [sample MakeWaitAny]
    std::vector<TestAwaitable> awaitables(3);

    auto wait_any = engine::MakeWaitAny(awaitables[0], awaitables[1], awaitables[2]);
    ASSERT_EQ(wait_any.GetNextId(), awaitables.size());

    EXPECT_EQ(wait_any.WaitUntil(engine::Deadline::Passed()), utils::unexpected(engine::WaitAnyError::kTimeout));

    for (std::size_t i = 0; i < awaitables.size(); ++i) {
        ASSERT_EQ(wait_any.GetSize(), awaitables.size() - i);
        awaitables[(i + 1) % awaitables.size()].SetReady();
        auto index = wait_any.Wait();
        ASSERT_TRUE(index.has_value());
        EXPECT_EQ(*index, (i + 1) % awaitables.size());
    }
    ASSERT_EQ(wait_any.GetSize(), 0);
    EXPECT_EQ(wait_any.Wait(), utils::unexpected(engine::WaitAnyError::kEmpty));
    /// [sample MakeWaitAny]
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
    ASSERT_EQ(wait_any.GetNextId(), all.size());

    EXPECT_EQ(wait_any.WaitUntil(engine::Deadline::Passed()), utils::unexpected(engine::WaitAnyError::kTimeout));

    for (std::size_t i = 0; i < all.size(); ++i) {
        ASSERT_EQ(wait_any.GetSize(), all.size() - i);
        all[(i + 3) % all.size()]->SetReady();
        auto index = wait_any.Wait();
        ASSERT_TRUE(index.has_value());
        EXPECT_EQ(*index, (i + 3) % all.size());
    }
    ASSERT_EQ(wait_any.GetSize(), 0);
    EXPECT_EQ(wait_any.Wait(), utils::unexpected(engine::WaitAnyError::kEmpty));
}

UTEST(WaitAnyContext, WaitAnyContextDynamicAppend) {
    std::vector<TestAwaitable> awaitables(10);

    engine::WaitAnyContext wait_any;
    ASSERT_EQ(wait_any.GetNextId(), 0);

    for (std::size_t i = 0; i < awaitables.size() / 2; ++i) {
        wait_any.Append(awaitables[i * 2]);
        wait_any.Append(awaitables[(i * 2) + 1]);
        ASSERT_EQ(wait_any.GetSize(), i + 2);
        ASSERT_EQ(wait_any.GetNextId(), (i + 1) * 2);
        EXPECT_EQ(wait_any.WaitUntil(engine::Deadline::Passed()), utils::unexpected(engine::WaitAnyError::kTimeout));

        awaitables[i * 2].SetReady();
        auto index = wait_any.Wait();
        ASSERT_TRUE(index.has_value());
        EXPECT_EQ(*index, i * 2);
    }

    for (std::size_t i = 0; i < awaitables.size() / 2; ++i) {
        ASSERT_EQ(wait_any.GetSize(), (awaitables.size() / 2) - i);
        EXPECT_EQ(wait_any.WaitUntil(engine::Deadline::Passed()), utils::unexpected(engine::WaitAnyError::kTimeout));

        awaitables[(i * 2) + 1].SetReady();
        auto index = wait_any.Wait();
        ASSERT_TRUE(index.has_value());
        EXPECT_EQ(*index, (i * 2) + 1);
    }
    ASSERT_EQ(wait_any.GetSize(), 0);
    EXPECT_EQ(wait_any.Wait(), utils::unexpected(engine::WaitAnyError::kEmpty));
}

UTEST(WaitAnyContext, AppendWithExplicitIndex) {
    TestAwaitable awaitable0;
    TestAwaitable awaitable1;
    TestAwaitable awaitable2;

    engine::WaitAnyContext wait_any;
    // Append with explicit non-sequential ids.
    wait_any.Append(std::uint64_t{100}, awaitable0);
    wait_any.Append(std::uint64_t{42}, awaitable1);
    wait_any.Append(std::uint64_t{999}, awaitable2);

    // GetNextId must not be affected by explicit-id appends.
    EXPECT_EQ(wait_any.GetNextId(), 0);
    EXPECT_EQ(wait_any.GetSize(), 3);

    // Force subscriptions.
    EXPECT_EQ(wait_any.WaitUntil(engine::Deadline::Passed()), utils::unexpected(engine::WaitAnyError::kTimeout));

    awaitable1.SetReady();
    auto index = wait_any.Wait();
    ASSERT_TRUE(index.has_value());
    EXPECT_EQ(*index, 42);

    awaitable2.SetReady();
    index = wait_any.Wait();
    ASSERT_TRUE(index.has_value());
    EXPECT_EQ(*index, 999);

    awaitable0.SetReady();
    index = wait_any.Wait();
    ASSERT_TRUE(index.has_value());
    EXPECT_EQ(*index, 100);

    EXPECT_EQ(wait_any.GetSize(), 0);
    EXPECT_EQ(wait_any.Wait(), utils::unexpected(engine::WaitAnyError::kEmpty));
}

UTEST(WaitAnyContext, AppendWithExplicitIndexMixedWithImplicit) {
    TestAwaitable awaitable_explicit;
    TestAwaitable awaitable_implicit0;
    TestAwaitable awaitable_implicit1;

    engine::WaitAnyContext wait_any;

    // Implicit appends advance GetNextId.
    wait_any.Append(awaitable_implicit0);
    EXPECT_EQ(wait_any.GetNextId(), 1);

    // Explicit-id append must NOT advance GetNextId.
    wait_any.Append(std::uint64_t{77}, awaitable_explicit);
    EXPECT_EQ(wait_any.GetNextId(), 1);

    // Another implicit append continues the sequence.
    wait_any.Append(awaitable_implicit1);
    EXPECT_EQ(wait_any.GetNextId(), 2);

    EXPECT_EQ(wait_any.GetSize(), 3);

    // Force subscriptions.
    EXPECT_EQ(wait_any.WaitUntil(engine::Deadline::Passed()), utils::unexpected(engine::WaitAnyError::kTimeout));

    awaitable_explicit.SetReady();
    auto index = wait_any.Wait();
    ASSERT_TRUE(index.has_value());
    EXPECT_EQ(*index, 77);

    awaitable_implicit0.SetReady();
    index = wait_any.Wait();
    ASSERT_TRUE(index.has_value());
    EXPECT_EQ(*index, 0);

    awaitable_implicit1.SetReady();
    index = wait_any.Wait();
    ASSERT_TRUE(index.has_value());
    EXPECT_EQ(*index, 1);

    EXPECT_EQ(wait_any.GetSize(), 0);
    EXPECT_EQ(wait_any.Wait(), utils::unexpected(engine::WaitAnyError::kEmpty));
}

UTEST(WaitAnyContext, AppendWithExplicitIndexEmptyAwaitable) {
    TestAwaitable awaitable;

    engine::WaitAnyContext wait_any;

    // Appending an already-ready (empty token) awaitable with explicit index is a no-op.
    awaitable.SetReady();
    wait_any.Append(std::uint64_t{55}, awaitable);

    // GetNextId must remain 0 (explicit-id append never advances it).
    EXPECT_EQ(wait_any.GetNextId(), 0);
    // GetSize must be 0 because the token is empty (awaitable is ready, returns empty token).
    EXPECT_EQ(wait_any.GetSize(), 0);

    EXPECT_EQ(wait_any.Wait(), utils::unexpected(engine::WaitAnyError::kEmpty));
}

UTEST(WaitAnyContext, AppendWithExplicitIndexAlreadyReadyAwaitable) {
    // Use a task that finishes immediately so its token is non-empty but ready.
    auto task = engine::AsyncNoTracing([] {});
    engine::Yield();  // Let the task finish.

    engine::WaitAnyContext wait_any;
    wait_any.Append(std::uint64_t{123}, task);

    EXPECT_EQ(wait_any.GetNextId(), 0);

    auto index = wait_any.Wait();
    ASSERT_TRUE(index.has_value());
    EXPECT_EQ(*index, 123);
}

UTEST(WaitAnyContext, MakeReadyAwaitableTokenWithWaitAnyContext) {
    // A struct that wraps MakeReadyAwaitableToken so it satisfies engine::Awaitable.
    struct ReadyAwaitable {
        engine::AwaitableToken GetAwaitableToken() noexcept { return engine::MakeReadyAwaitableToken(); }
    };

    ReadyAwaitable ready;
    engine::WaitAnyContext wait_any;
    wait_any.Append(ready);

    EXPECT_EQ(wait_any.GetNextId(), 1);
    EXPECT_EQ(wait_any.GetSize(), 1);

    // A ready awaitable should be returned immediately.
    auto index = wait_any.Wait();
    ASSERT_TRUE(index.has_value());
    EXPECT_EQ(*index, 0);

    EXPECT_EQ(wait_any.GetSize(), 0);
    EXPECT_EQ(wait_any.Wait(), utils::unexpected(engine::WaitAnyError::kEmpty));
}

UTEST(WaitAnyContext, AppendWithExplicitIndexDuplicateIndexAllowed) {
    // The API does not forbid duplicate explicit indexes; both should be returned.
    TestAwaitable awaitable0;
    TestAwaitable awaitable1;

    engine::WaitAnyContext wait_any;
    wait_any.Append(std::uint64_t{7}, awaitable0);
    wait_any.Append(std::uint64_t{7}, awaitable1);

    EXPECT_EQ(wait_any.GetSize(), 2);

    // Force subscriptions.
    EXPECT_EQ(wait_any.WaitUntil(engine::Deadline::Passed()), utils::unexpected(engine::WaitAnyError::kTimeout));

    awaitable0.SetReady();
    auto index = wait_any.Wait();
    ASSERT_TRUE(index.has_value());
    EXPECT_EQ(*index, 7);

    awaitable1.SetReady();
    index = wait_any.Wait();
    ASSERT_TRUE(index.has_value());
    EXPECT_EQ(*index, 7);

    EXPECT_EQ(wait_any.GetSize(), 0);
    EXPECT_EQ(wait_any.Wait(), utils::unexpected(engine::WaitAnyError::kEmpty));
}

UTEST(WaitAnyContext, AppendWithExplicitIndexSlotMap) {
    /// [sample WaitAnyContext SlotMap]
    utils::SlotMap<engine::TaskWithResult<int>> tasks;

    // Spawn several tasks and register each one in the WaitAnyContext using its
    // SlotMap index as the explicit WaitAnyContext index.
    engine::WaitAnyContext wait_any;
    for (int value : {1, 2, 3, 4}) {
        auto [task, index] = tasks.emplace(engine::AsyncNoTracing([value] { return value; }));
        wait_any.Append(index, task);
    }

    // Collect results as tasks finish, in completion order.
    int sum = 0;
    while (!tasks.empty()) {
        const auto index_opt = wait_any.Wait();
        ASSERT_TRUE(index_opt.has_value());

        const std::size_t index = *index_opt;
        sum += tasks[index].Get();
        tasks.erase(index);
    }

    EXPECT_EQ(sum, 1 + 2 + 3 + 4);
    /// [sample WaitAnyContext SlotMap]
}

USERVER_NAMESPACE_END
