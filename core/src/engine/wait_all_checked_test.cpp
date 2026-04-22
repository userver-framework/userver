#include <userver/utest/utest.hpp>

#include <userver/engine/async.hpp>
#include <userver/engine/future.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/wait_all_checked.hpp>
#include <userver/utest/stress.hpp>
#include <userver/utils/impl/userver_experiments.hpp>
#include <userver/utils/rand.hpp>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

namespace {

engine::TaskWithResult<void> SlowSuccessfulTask() {
    return engine::AsyncNoSpan([] {
        engine::InterruptibleSleepFor(utest::kMaxTestWaitTime);
        engine::current_task::CancellationPoint();
        FAIL() << "This task should have been cancelled";
    });
}

engine::TaskWithResult<void> FastFailingTask() {
    return engine::AsyncNoSpan([] {
        engine::InterruptibleSleepFor(20ms);
        throw std::runtime_error{"failfast_exception"};
    });
}

engine::TaskWithResult<void> FastSuccessfulTask() {
    return engine::AsyncNoSpan([] { engine::InterruptibleSleepFor(20ms); });
}

engine::TaskWithResult<int> FastSuccessfulTask(int i) {
    return engine::AsyncNoSpan([i] {
        engine::InterruptibleSleepFor(20ms);
        return i;
    });
}

}  // namespace

class WaitAllChecked : public ::testing::Test {
public:
    void SetUp() override { scope_.Set(utils::impl::kWaitAllCheckedUpgradeExperiment, true); }

private:
    utils::impl::UserverExperimentsScope scope_;
};

UTEST_F(WaitAllChecked, JustWorksVectorTasks) {
    static constexpr std::size_t kTaskCount = 4;

    std::vector<engine::TaskWithResult<void>> tasks;
    tasks.reserve(kTaskCount);

    for (std::size_t i = 0; i < 4; ++i) {
        tasks.push_back(FastSuccessfulTask());
    }

    UEXPECT_NO_THROW(engine::WaitAllChecked(tasks));

    for (const auto& task : tasks) {
        ASSERT_TRUE(task.IsValid());
        EXPECT_TRUE(task.IsFinished());
    }
}

UTEST_F(WaitAllChecked, JustWorksVariadicTasks) {
    static constexpr std::size_t kTaskCount = 3;

    std::vector<engine::TaskWithResult<void>> tasks;
    tasks.reserve(kTaskCount);

    for (std::size_t i = 0; i < kTaskCount; ++i) {
        tasks.push_back(FastSuccessfulTask());
    }

    UEXPECT_NO_THROW(engine::WaitAllChecked(tasks[0], tasks[1], tasks[2]));

    for (const auto& task : tasks) {
        ASSERT_TRUE(task.IsValid());
        EXPECT_TRUE(task.IsFinished());
    }
}

UTEST_F_MT(WaitAllChecked, EarlyThrow, 4) {
    std::vector<engine::TaskWithResult<void>> tasks;
    tasks.reserve(GetThreadCount() + 1);

    for (std::size_t i = 0; i < GetThreadCount(); ++i) {
        tasks.push_back(SlowSuccessfulTask());
    }
    tasks.push_back(FastFailingTask());

    UEXPECT_THROW(engine::WaitAllChecked(tasks), std::runtime_error);
}

UTEST_F(WaitAllChecked, InvalidTask) {
    std::vector<engine::TaskWithResult<void>> tasks;
    tasks.emplace_back();
    UEXPECT_NO_THROW(engine::WaitAllChecked(tasks));
}

UTEST_F(WaitAllChecked, ValidAndInvalidTasks) {
    std::vector<engine::TaskWithResult<void>> tasks;
    tasks.push_back(FastSuccessfulTask());
    for (std::size_t i = 0; i < 3; ++i) {
        tasks.emplace_back();
    }
    UEXPECT_NO_THROW(engine::WaitAllChecked(tasks));
    ASSERT_TRUE(tasks[0].IsValid());
    EXPECT_TRUE(tasks[0].IsFinished());
}

UTEST_F(WaitAllChecked, Cancellation) {
    constexpr std::size_t kTaskCount = 3;

    std::vector<engine::TaskWithResult<void>> tasks;
    tasks.reserve(kTaskCount);
    for (std::size_t i = 0; i < kTaskCount; ++i) {
        tasks.push_back(SlowSuccessfulTask());
    }

    engine::current_task::SetDeadline(engine::Deadline::Passed());

    UEXPECT_THROW(engine::WaitAllChecked(tasks), engine::WaitInterruptedException);
}

UTEST_F(WaitAllChecked, SequentialWakeups) {
    constexpr std::size_t kTaskCount = 10;

    engine::SingleConsumerEvent events[kTaskCount];

    std::vector<engine::TaskWithResult<void>> tasks;
    tasks.reserve(kTaskCount);
    for (std::size_t i = 0; i < kTaskCount; ++i) {
        tasks.push_back(engine::AsyncNoSpan([i, &events] {
            if (i + 1 < kTaskCount) {
                ASSERT_TRUE(events[i + 1].WaitForEventFor(utest::kMaxTestWaitTime));
            }
            events[i].Send();
        }));
    }

    engine::WaitAllChecked(tasks);
}

UTEST_F(WaitAllChecked, TaskWithResult) {
    constexpr std::size_t kTaskCount = 10;

    std::vector<engine::TaskWithResult<int>> tasks;
    tasks.reserve(kTaskCount);
    for (std::size_t i = 0; i < kTaskCount; ++i) {
        tasks.push_back(FastSuccessfulTask(i));
    }

    engine::WaitAllChecked(tasks);
    for (std::size_t i = 0; i < kTaskCount; ++i) {
        auto& task = tasks[i];
        ASSERT_TRUE(task.IsValid());
        EXPECT_TRUE(task.IsFinished());
        EXPECT_EQ(task.Get(), i);
    }
}

UTEST_F(WaitAllChecked, HeterogenousWait) {
    constexpr int kExpectedValue = 42;

    auto task = FastSuccessfulTask();
    engine::Promise<int> promise;
    auto future = promise.get_future();

    auto notifier_task = engine::AsyncNoSpan([&] {
        engine::SleepFor(20ms);
        promise.set_value(kExpectedValue);
    });

    UEXPECT_NO_THROW(engine::WaitAllChecked(task, future));

    ASSERT_TRUE(task.IsValid());
    EXPECT_TRUE(task.IsFinished());
    EXPECT_EQ(future.wait_for(0s), engine::FutureStatus::kReady);
    EXPECT_EQ(future.get(), kExpectedValue);
}

UTEST_F(WaitAllChecked, DeadlineSuccess) {
    auto task = FastSuccessfulTask();
    EXPECT_EQ(engine::WaitAllCheckedFor(utest::kMaxTestWaitTime, task), engine::FutureStatus::kReady);
    EXPECT_TRUE(task.IsFinished());
}

UTEST_F(WaitAllChecked, DeadlineTimeout) {
    auto task = SlowSuccessfulTask();
    EXPECT_EQ(engine::WaitAllCheckedFor(10ms, task), engine::FutureStatus::kTimeout);
    EXPECT_FALSE(task.IsFinished());
}

UTEST_F(WaitAllChecked, DeadlineCancelled) {
    auto task = SlowSuccessfulTask();
    engine::current_task::SetDeadline(engine::Deadline::FromDuration(10ms));
    EXPECT_EQ(engine::WaitAllCheckedFor(utest::kMaxTestWaitTime, task), engine::FutureStatus::kCancelled);
}

UTEST_F(WaitAllChecked, DeadlineCancelledBefore) {
    auto task = SlowSuccessfulTask();
    engine::current_task::SetDeadline(engine::Deadline::Passed());
    EXPECT_EQ(engine::WaitAllCheckedFor(utest::kMaxTestWaitTime, task), engine::FutureStatus::kCancelled);
}

UTEST_F(WaitAllChecked, DeadlineTimeoutUntil) {
    auto task = SlowSuccessfulTask();
    EXPECT_EQ(
        engine::WaitAllCheckedUntil(std::chrono::steady_clock::now() + 10ms, task),
        engine::FutureStatus::kTimeout
    );
}

UTEST_F_MT(WaitAllChecked, ExceptionStressTest, 16) {
    ASSERT_TRUE(utils::impl::kWaitAllCheckedUpgradeExperiment.IsEnabled());
    for (auto _ : utest::StressLoop()) {
        const std::size_t task_count = utils::RandRange(100u, 500u);
        const std::size_t failing = utils::RandRange(std::size_t{0}, task_count);
        std::vector<engine::Task> tasks;
        tasks.reserve(task_count);
        for (std::size_t i = 0; i < failing; ++i) {
            tasks.push_back(FastSuccessfulTask());
        }
        tasks.push_back(FastFailingTask());
        for (std::size_t i = failing + 1; i < task_count; ++i) {
            tasks.push_back(FastSuccessfulTask());
        }
        UEXPECT_THROW(engine::WaitAllChecked(tasks), std::runtime_error);
    }
}

USERVER_NAMESPACE_END
