#include <userver/utest/utest.hpp>

#include <sys/param.h>

#include <atomic>
#include <deque>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <vector>

#include <userver/concurrent/variable.hpp>
#include <userver/dist_lock/dist_lock_settings.hpp>
#include <userver/dist_lock/dist_lock_strategy.hpp>
#include <userver/dist_lock/dist_locked_task.hpp>
#include <userver/dist_lock/dist_locked_worker.hpp>
#include <userver/engine/condition_variable.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/logging/log.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/utils/mock_now.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::chrono::milliseconds kAttemptInterval{10};
constexpr std::chrono::milliseconds kLockTtl{100};
constexpr auto kAttemptTimeout = 5 * kAttemptInterval;

const std::string kWorkerName = "test";

dist_lock::DistLockSettings MakeSettings() {
    return {kAttemptInterval, kAttemptInterval, kLockTtl, kAttemptInterval, kAttemptInterval};
}

/// [Sample distlock strategy]
class MockDistLockStrategy final : public dist_lock::DistLockStrategyBase {
public:
    ~MockDistLockStrategy() override { EXPECT_FALSE(IsLocked()); }

    void Acquire(std::chrono::milliseconds, const std::string& locker_id) override {
        UASSERT(!locker_id.empty());
        const utils::FastScopeGuard count_attempt([this]() noexcept { attempts_++; });
        auto locked_by = locked_by_var_.Lock();
        if (!locked_by->empty() && *locked_by != locker_id) {
            throw dist_lock::LockIsAcquiredByAnotherHostException();
        }
        if (!allowed_) {
            throw std::runtime_error("not allowed");
        }
        *locked_by = locker_id;
    }

    void Prolong(std::chrono::milliseconds lock_ttl, const std::string& locker_id) override {
        Acquire(lock_ttl, locker_id);
        prolongs_++;
    }

    void Release(const std::string& locker_id) override {
        auto locked_by = locked_by_var_.Lock();
        if (*locked_by == locker_id) {
            locked_by->clear();
        }
    }

    bool IsLocked() {
        auto locked_by = locked_by_var_.Lock();
        return !locked_by->empty();
    }

    void Allow(bool allowed) { allowed_ = allowed; }

    void SetLockedBy(const std::string& whom) {
        auto locked_by = locked_by_var_.Lock();
        *locked_by = whom;
    }

    size_t GetAttemptsCount() const { return attempts_; }

    size_t GetProlongsCount() const { return prolongs_; }

private:
    concurrent::Variable<std::string> locked_by_var_;
    std::atomic<bool> allowed_{false};
    std::atomic<size_t> attempts_{0};
    std::atomic<size_t> prolongs_{0};
};
/// [Sample distlock strategy]

auto MakeMockStrategy() { return std::make_shared<MockDistLockStrategy>(); }

// A strategy that intentionally does NOT override Prolong(), so refreshes fall
// back to the default DistLockStrategyBase::Prolong() (which calls Acquire()).
class DefaultProlongStrategy final : public dist_lock::DistLockStrategyBase {
public:
    ~DefaultProlongStrategy() override {
        auto locked_by = locked_by_var_.Lock();
        EXPECT_TRUE(locked_by->empty());
    }

    void Acquire(std::chrono::milliseconds, const std::string& locker_id) override {
        UASSERT(!locker_id.empty());
        acquires_++;
        auto locked_by = locked_by_var_.Lock();
        if (!locked_by->empty() && *locked_by != locker_id) {
            throw dist_lock::LockIsAcquiredByAnotherHostException();
        }
        *locked_by = locker_id;
    }

    void Release(const std::string& locker_id) override {
        auto locked_by = locked_by_var_.Lock();
        if (*locked_by == locker_id) {
            locked_by->clear();
        }
    }

    size_t GetAcquiresCount() const { return acquires_; }

private:
    concurrent::Variable<std::string> locked_by_var_;
    std::atomic<size_t> acquires_{0};
};

// Checks and fallback acquisitions are answered by the test, so intermediate
// states can be inspected without racing the next acquisition attempt.
class ScriptedFallbackStrategy final : public dist_lock::DistLockStrategyBase {
public:
    void Acquire(std::chrono::milliseconds, const std::string&) override {
        throw std::logic_error("unexpected legacy acquisition");
    }

    dist_lock::LockOperationResult ExecuteOperation(const std::string&, const dist_lock::LockOperationOptions& options)
        override {
        std::unique_lock lock(mutex_);
        if (options.operation == dist_lock::LockOperation::kProlong) {
            ++prolongs_;
            request_cv_.NotifyAll();
            return dist_lock::LockOperationResult::kAcquired;
        }
        if (options.operation == dist_lock::LockOperation::kAcquire) {
            return dist_lock::LockOperationResult::kAcquired;
        }
        ++requests_count_;
        requests_.push_back(options);
        request_cv_.NotifyAll();
        if (!reply_cv_.Wait(lock, [this] { return !replies_.empty(); })) {
            engine::current_task::CancellationPoint();
            throw std::runtime_error("cancelled fallback request");
        }
        const auto reply = replies_.front();
        replies_.pop_front();
        if (!reply) {
            throw std::runtime_error("heartbeat query failed");
        }
        return reply.value();
    }

    void Release(const std::string& locker_id) override { Release(locker_id, {}); }

    void Release(const std::string&, const dist_lock::ReleaseOptions& options) override {
        const std::lock_guard lock(mutex_);
        releases_.push_back(options);
        request_cv_.NotifyAll();
    }

    std::optional<dist_lock::LockOperationOptions> WaitForRequest() {
        std::unique_lock lock(mutex_);
        if (!request_cv_.WaitFor(lock, utest::kMaxTestWaitTime, [this] { return !requests_.empty(); })) {
            return std::nullopt;
        }
        const auto request = requests_.front();
        requests_.pop_front();
        return request;
    }

    void Reply(std::optional<dist_lock::LockOperationResult> result) {
        const std::lock_guard lock(mutex_);
        replies_.push_back(result);
        reply_cv_.NotifyAll();
    }

    bool WaitForProlongs(std::size_t count) {
        std::unique_lock lock(mutex_);
        return request_cv_.WaitFor(lock, utest::kMaxTestWaitTime, [this, count] { return prolongs_ >= count; });
    }

    bool WaitForReleases(std::size_t count) {
        std::unique_lock lock(mutex_);
        return request_cv_.WaitFor(lock, utest::kMaxTestWaitTime, [this, count] { return releases_.size() >= count; });
    }

    std::vector<dist_lock::ReleaseOptions> GetReleases() {
        const std::lock_guard lock(mutex_);
        return releases_;
    }

    std::size_t GetProlongsCount() const { return prolongs_; }

    std::size_t GetRequestsCount() const { return requests_count_; }

private:
    engine::Mutex mutex_;
    engine::ConditionVariable request_cv_;
    engine::ConditionVariable reply_cv_;
    std::deque<dist_lock::LockOperationOptions> requests_;
    std::deque<std::optional<dist_lock::LockOperationResult>> replies_;
    std::atomic<std::size_t> requests_count_{0};
    std::atomic<std::size_t> prolongs_{0};
    std::vector<dist_lock::ReleaseOptions> releases_;
};

dist_lock::DistLockSettings MakeFallbackSettings() {
    auto settings = MakeSettings();
    settings.fallback_policy = dist_lock::DistLockFallbackSettings{.fallback_after_checks = 3};
    settings.is_host_deprioritized = true;
    return settings;
}

class DistLockWorkload {
public:
    explicit DistLockWorkload(bool abort_on_cancel = false)
        : abort_on_cancel_(abort_on_cancel)
    {}

    bool IsLocked() const { return is_locked_; }

    bool WaitForLocked(bool locked, std::chrono::milliseconds ms) {
        std::unique_lock<engine::Mutex> lock(mutex_);
        return cv_.WaitFor(lock, ms, [locked, this] { return locked == IsLocked(); });
    }

    void SetWorkLoopOn(bool val) { work_loop_on_.store(val); }
    size_t GetStartedWorkCount() const { return work_start_count_.load(); }
    size_t GetFinishedWorkCount() const { return work_finish_count_.load(); }

    void Work() {
        LOG_DEBUG() << "work begin";
        SetLocked(true);
        work_start_count_++;
        LOG_DEBUG() << "work begin after SetLocked(true)";

        try {
            bool work_loop_on = work_loop_on_;
            while (work_loop_on && !engine::current_task::IsCancelRequested()) {
                LOG_DEBUG() << "work loop";
                engine::InterruptibleSleepFor(std::chrono::milliseconds(50));
                work_loop_on = work_loop_on_;
            }

            if (work_loop_on && abort_on_cancel_) {
                engine::current_task::CancellationPoint();
            }
        } catch (...) {
            SetLocked(false);
            throw;
        }

        LOG_DEBUG() << "work end";
        work_finish_count_++;
        SetLocked(false);
        LOG_DEBUG() << "work end after SetLocked(false)";
    }

private:
    void SetLocked(bool locked) {
        const std::lock_guard<engine::Mutex> lock(mutex_);
        is_locked_ = locked;
        cv_.NotifyAll();
    }

    const bool abort_on_cancel_;
    std::atomic<bool> is_locked_{false};
    std::atomic<bool> work_loop_on_{true};
    engine::Mutex mutex_;
    engine::ConditionVariable cv_;
    std::atomic<size_t> work_start_count_{0};
    std::atomic<size_t> work_finish_count_{0};
};

}  // namespace

UTEST(LockedWorker, Noop) {
    const dist_lock::DistLockedWorker locked_worker(kWorkerName, [] {}, MakeMockStrategy(), MakeSettings());
}

UTEST_MT(LockedWorker, StartStop, 3) {
    auto strategy = MakeMockStrategy();
    DistLockWorkload work;
    dist_lock::DistLockedWorker locked_worker(kWorkerName, [&] { work.Work(); }, strategy, MakeSettings());
    EXPECT_FALSE(work.IsLocked());

    locked_worker.Start();
    EXPECT_FALSE(work.WaitForLocked(true, kAttemptTimeout));

    strategy->Allow(true);
    EXPECT_TRUE(work.WaitForLocked(true, utest::kMaxTestWaitTime));

    locked_worker.Stop();
}

UTEST_MT(LockedWorker, Watchdog, 3) {
    auto strategy = MakeMockStrategy();
    DistLockWorkload work;
    dist_lock::DistLockedWorker locked_worker(kWorkerName, [&] { work.Work(); }, strategy, MakeSettings());

    locked_worker.Start();
    strategy->Allow(true);
    EXPECT_TRUE(work.WaitForLocked(true, utest::kMaxTestWaitTime));

    strategy->Allow(false);
    EXPECT_TRUE(work.WaitForLocked(false, utest::kMaxTestWaitTime));

    locked_worker.Stop();
}

UTEST_MT(LockedWorker, NotEnabled, 3) {
    auto strategy = MakeMockStrategy();
    DistLockWorkload work;
    auto settings = MakeSettings();
    settings.is_enabled = false;
    dist_lock::DistLockedWorker locked_worker(kWorkerName, [&] { work.Work(); }, strategy, settings);

    locked_worker.Start();
    strategy->Allow(true);
    EXPECT_FALSE(work.WaitForLocked(true, std::chrono::seconds{1}));

    locked_worker.Stop();
}

UTEST_MT(LockedWorker, SetNotEnabled, 3) {
    auto strategy = MakeMockStrategy();
    DistLockWorkload work;
    auto settings = MakeSettings();

    dist_lock::DistLockedWorker locked_worker(kWorkerName, [&] { work.Work(); }, strategy, settings);

    locked_worker.Start();
    strategy->Allow(true);
    EXPECT_TRUE(work.WaitForLocked(true, utest::kMaxTestWaitTime));

    settings.is_enabled = false;
    locked_worker.UpdateSettings(settings);
    work.SetWorkLoopOn(false);

    EXPECT_TRUE(work.WaitForLocked(false, utest::kMaxTestWaitTime));
    work.SetWorkLoopOn(true);
    // Expect the disabled worker to not reacquire the lock.
    // In case of a bug in DistLockedWorker, kAttemptTimeout will be enough to "probably" discover the bug.
    // In case of no bug, the test will not become flaky due to a smaller timeout.
    EXPECT_FALSE(work.WaitForLocked(true, kAttemptTimeout));

    locked_worker.Stop();
}

UTEST_MT(LockedWorker, OkAfterFail, 3) {
    auto strategy = MakeMockStrategy();
    DistLockWorkload work;
    dist_lock::DistLockedWorker locked_worker(kWorkerName, [&] { work.Work(); }, strategy, MakeSettings());

    locked_worker.Start();
    EXPECT_FALSE(work.WaitForLocked(true, kAttemptTimeout));
    auto fail_count = strategy->GetAttemptsCount();
    EXPECT_LT(0, fail_count);
    EXPECT_FALSE(work.IsLocked());

    strategy->Allow(true);
    EXPECT_TRUE(work.WaitForLocked(true, utest::kMaxTestWaitTime));
    EXPECT_LT(fail_count, strategy->GetAttemptsCount());

    locked_worker.Stop();
}

// TODO: TAXICOMMON-1059
UTEST_MT(LockedWorker, DISABLED_IN_MAC_OS_TEST_NAME(OkFailOk), 3) {
    auto strategy = MakeMockStrategy();
    DistLockWorkload work;
    dist_lock::DistLockedWorker locked_worker(kWorkerName, [&] { work.Work(); }, strategy, MakeSettings());

    locked_worker.Start();
    strategy->Allow(true);
    EXPECT_TRUE(work.WaitForLocked(true, utest::kMaxTestWaitTime));

    strategy->Allow(false);
    auto attempts_count = strategy->GetAttemptsCount();
    EXPECT_LT(0, attempts_count);
    EXPECT_FALSE(work.WaitForLocked(false, kAttemptTimeout));

    auto attempts_count2 = strategy->GetAttemptsCount();
    EXPECT_LT(attempts_count, attempts_count2);

    strategy->Allow(true);
    // FIXME
    EXPECT_FALSE(work.WaitForLocked(false, kAttemptTimeout));
    auto attempts_count3 = strategy->GetAttemptsCount();
    EXPECT_LT(attempts_count2, attempts_count3);

    locked_worker.Stop();
}

UTEST_MT(LockedWorker, LockedByOther, 3) {
    auto strategy = MakeMockStrategy();
    DistLockWorkload work;
    dist_lock::DistLockedWorker locked_worker(kWorkerName, [&] { work.Work(); }, strategy, MakeSettings());

    locked_worker.Start();
    strategy->Allow(true);
    EXPECT_TRUE(work.WaitForLocked(true, utest::kMaxTestWaitTime));

    strategy->SetLockedBy("me");
    EXPECT_TRUE(work.WaitForLocked(false, utest::kMaxTestWaitTime));

    strategy->Release("me");
    EXPECT_TRUE(work.WaitForLocked(false, utest::kMaxTestWaitTime));

    locked_worker.Stop();
}

UTEST_MT(LockedTask, Smoke, 3) {
    auto strategy = MakeMockStrategy();
    DistLockWorkload work;
    const dist_lock::DistLockedTask locked_task(kWorkerName, [&] { work.Work(); }, strategy, MakeSettings());

    EXPECT_EQ(0U, work.GetFinishedWorkCount());
    strategy->Allow(true);
    EXPECT_TRUE(work.WaitForLocked(true, kAttemptTimeout));

    work.SetWorkLoopOn(false);
    strategy->Allow(false);
    locked_task.WaitFor(utest::kMaxTestWaitTime);
    EXPECT_TRUE(locked_task.GetState() == engine::Task::State::kCompleted);
    EXPECT_EQ(1U, work.GetFinishedWorkCount());
}

UTEST_MT(LockedTask, SingleAttempt, 3) {
    auto strategy = MakeMockStrategy();
    const DistLockWorkload work;
    std::atomic<size_t> counter{0};
    dist_lock::DistLockedTask locked_task(
        kWorkerName,
        [&] {
            counter++;
            throw std::runtime_error("123");
        },
        strategy,
        MakeSettings(),
        dist_lock::DistLockWaitingMode::kWait,
        dist_lock::DistLockRetryMode::kSingleAttempt
    );

    EXPECT_EQ(0U, work.GetFinishedWorkCount());
    strategy->Allow(true);

    locked_task.WaitFor(utest::kMaxTestWaitTime);
    ASSERT_TRUE(locked_task.IsFinished());
    try {
        locked_task.Get();
        FAIL() << "Should have thrown";
    } catch (const std::runtime_error& e) {
        EXPECT_EQ(e.what(), std::string{"123"});
    }
    EXPECT_EQ(counter.load(), 1);

    EXPECT_EQ(0U, work.GetFinishedWorkCount());
}

UTEST_MT(LockedTask, Fail, 3) {
    auto settings = MakeSettings();
    settings.prolong_interval += settings.lock_ttl;  // make watchdog fire

    auto strategy = MakeMockStrategy();
    DistLockWorkload work(true);
    const dist_lock::DistLockedTask locked_task(kWorkerName, [&] { work.Work(); }, strategy, settings);

    EXPECT_EQ(0U, work.GetStartedWorkCount());
    EXPECT_EQ(0U, work.GetFinishedWorkCount());
    strategy->Allow(true);

    EXPECT_TRUE(work.WaitForLocked(true, kAttemptTimeout));
    locked_task.WaitFor(settings.prolong_interval + kAttemptTimeout);
    EXPECT_FALSE(locked_task.IsFinished());
    EXPECT_TRUE(work.WaitForLocked(true, utest::kMaxTestWaitTime));
    EXPECT_TRUE(work.WaitForLocked(false, utest::kMaxTestWaitTime));

    EXPECT_LE(1U, work.GetStartedWorkCount());
    EXPECT_EQ(0U, work.GetFinishedWorkCount());
}

UTEST_MT(LockedTask, NoWait, 3) {
    auto settings = MakeSettings();

    auto strategy = MakeMockStrategy();
    strategy->SetLockedBy("me");

    DistLockWorkload work(true);
    const dist_lock::DistLockedTask
        locked_task(kWorkerName, [&] { work.Work(); }, strategy, settings, dist_lock::DistLockWaitingMode::kNoWait);

    engine::InterruptibleSleepFor(3 * settings.prolong_interval);

    EXPECT_EQ(1, strategy->GetAttemptsCount());

    EXPECT_TRUE(locked_task.IsFinished());
    EXPECT_EQ(0U, work.GetStartedWorkCount());
    EXPECT_EQ(0U, work.GetFinishedWorkCount());
    strategy->Release("me");
}

UTEST_MT(LockedTask, NoWaitAcquire, 3) {
    auto strategy = MakeMockStrategy();
    DistLockWorkload work;

    EXPECT_EQ(0U, work.GetFinishedWorkCount());
    strategy->Allow(true);

    const dist_lock::DistLockedTask locked_task(
        kWorkerName,
        [&] { work.Work(); },
        strategy,
        MakeSettings(),
        dist_lock::DistLockWaitingMode::kNoWait
    );

    EXPECT_TRUE(work.WaitForLocked(true, kAttemptTimeout));

    work.SetWorkLoopOn(false);
    locked_task.WaitFor(utest::kMaxTestWaitTime);

    EXPECT_TRUE(locked_task.GetState() == engine::Task::State::kCompleted);
    EXPECT_EQ(1U, work.GetFinishedWorkCount());
}

UTEST(LockedTask, MultipleWorkers) {
    auto strategy = MakeMockStrategy();
    DistLockWorkload work;

    EXPECT_EQ(0, work.GetStartedWorkCount());
    EXPECT_EQ(0, work.GetFinishedWorkCount());
    strategy->Allow(true);

    const dist_lock::DistLockedTask first(kWorkerName, [&] { work.Work(); }, strategy, MakeSettings());

    EXPECT_TRUE(work.WaitForLocked(true, kAttemptTimeout));
    EXPECT_EQ(1, work.GetStartedWorkCount());

    const dist_lock::DistLockedTask
        second(kWorkerName, [&] { work.Work(); }, strategy, MakeSettings(), dist_lock::DistLockWaitingMode::kNoWait);

    second.WaitFor(kAttemptTimeout);
    EXPECT_TRUE(second.GetState() == engine::Task::State::kCompleted);
    EXPECT_EQ(1, work.GetStartedWorkCount());

    work.SetWorkLoopOn(false);
    first.WaitFor(utest::kMaxTestWaitTime);
    second.WaitFor(utest::kMaxTestWaitTime);

    EXPECT_EQ(1, work.GetFinishedWorkCount());
}

std::shared_ptr<dist_lock::DistLockStrategyBase> GetSomeDistLockStrategyForTheSample() {
    auto strategy = std::make_shared<MockDistLockStrategy>();
    strategy->Allow(true);
    return strategy;
}

UTEST_MT(LockedTask, RetryExample, 3) {
    /// [Sample distributed locked task Retry]
    auto strategy = GetSomeDistLockStrategyForTheSample();
    std::atomic<std::size_t> counter = 0;
    dist_lock::DistLockedTask locked_task(
        "example",
        [&]() {
            counter++;
            if (counter < 5) {
                throw std::runtime_error("");
            }
        },
        strategy
    );

    UEXPECT_NO_THROW(locked_task.Get());
    EXPECT_EQ(counter, 5);
    /// [Sample distributed locked task Retry]
}

UTEST_MT(LockedTask, SingleAttemptExample, 3) {
    /// [Sample distributed locked task SingleAttempt]
    auto strategy = GetSomeDistLockStrategyForTheSample();
    std::atomic<std::size_t> counter = 0;
    dist_lock::DistLockedTask locked_task(
        "example",
        [&]() {
            counter++;
            throw std::runtime_error("123");
        },
        strategy,
        /*default settings*/ {},
        dist_lock::DistLockWaitingMode::kWait,
        dist_lock::DistLockRetryMode::kSingleAttempt
    );

    try {
        locked_task.Get();
        FAIL() << "Should have thrown";
    } catch (const std::runtime_error& exception) {
        EXPECT_EQ(exception.what(), std::string("123"));
    }
    EXPECT_EQ(counter, 1);
    /// [Sample distributed locked task SingleAttempt]
}

UTEST_MT(LockedWorker, ProlongUsedForRefresh, 3) {
    auto strategy = MakeMockStrategy();
    DistLockWorkload work;
    dist_lock::DistLockedWorker locked_worker(kWorkerName, [&] { work.Work(); }, strategy, MakeSettings());

    locked_worker.Start();
    strategy->Allow(true);
    EXPECT_TRUE(work.WaitForLocked(true, utest::kMaxTestWaitTime));

    for (int i = 0; i < 100 && strategy->GetProlongsCount() < 2; ++i) {
        engine::InterruptibleSleepFor(kAttemptInterval);
    }

    EXPECT_LE(2U, strategy->GetProlongsCount());
    EXPECT_LT(strategy->GetProlongsCount(), strategy->GetAttemptsCount());

    locked_worker.Stop();
}

UTEST_MT(LockedWorker, DefaultProlongRoutesThroughAcquire, 3) {
    auto strategy = std::make_shared<DefaultProlongStrategy>();
    DistLockWorkload work;
    dist_lock::DistLockedWorker locked_worker(kWorkerName, [&] { work.Work(); }, strategy, MakeSettings());

    locked_worker.Start();
    EXPECT_TRUE(work.WaitForLocked(true, utest::kMaxTestWaitTime));

    const auto acquires_after_take = strategy->GetAcquiresCount();
    for (int i = 0; i < 100 && strategy->GetAcquiresCount() <= acquires_after_take; ++i) {
        engine::InterruptibleSleepFor(kAttemptInterval);
    }
    EXPECT_LT(acquires_after_take, strategy->GetAcquiresCount());

    locked_worker.Stop();
}

UTEST(LockedWorker, LegacyStrategyResultAdapter) {
    auto strategy = MakeMockStrategy();
    strategy->Allow(true);
    dist_lock::DistLockStrategyBase& base = *strategy;
    using dist_lock::LockOperation;
    using dist_lock::LockOperationResult;

    EXPECT_EQ(
        base.ExecuteOperation("owner", {.lock_ttl = kLockTtl, .operation = LockOperation::kCheck}),
        LockOperationResult::kUnsupported
    );
    EXPECT_EQ(
        base.ExecuteOperation("owner", {.lock_ttl = kLockTtl, .operation = LockOperation::kFallback}),
        LockOperationResult::kUnsupported
    );
    EXPECT_EQ(strategy->GetAttemptsCount(), 0);
    EXPECT_EQ(base.ExecuteOperation("owner", {.lock_ttl = kLockTtl}), LockOperationResult::kAcquired);
    EXPECT_EQ(
        base.ExecuteOperation("owner", {.lock_ttl = kLockTtl, .operation = LockOperation::kProlong}),
        LockOperationResult::kAcquired
    );
    EXPECT_EQ(strategy->GetProlongsCount(), 1);
    EXPECT_EQ(base.ExecuteOperation("other-owner", {.lock_ttl = kLockTtl}), LockOperationResult::kBusy);
    base.Release("owner");
    EXPECT_FALSE(strategy->IsLocked());

    strategy->Allow(false);
    UEXPECT_THROW(base.ExecuteOperation("owner", {.lock_ttl = kLockTtl}), std::runtime_error);
}

UTEST(LockedWorker, DisabledFallbackDoesNotProbe) {
    auto strategy = std::make_shared<ScriptedFallbackStrategy>();
    auto settings = MakeFallbackSettings();
    settings.is_enabled = false;
    std::size_t work_count = 0;
    dist_lock::DistLockedWorker worker(kWorkerName, [&] { ++work_count; }, strategy, settings);

    EXPECT_TRUE(worker.IsCancelAdvised());
    worker.RunOnce();
    EXPECT_EQ(strategy->GetRequestsCount(), 0);
    EXPECT_EQ(work_count, 0);
}

UTEST_MT(LockedWorker, ReleaseHistoryFollowsSharedFallbackPolicy, 3) {
    auto strategy = std::make_shared<ScriptedFallbackStrategy>();
    auto settings = MakeSettings();
    DistLockWorkload work;
    dist_lock::DistLockedWorker worker(kWorkerName, [&] { work.Work(); }, strategy, settings);
    const utils::FastScopeGuard stop([&]() noexcept { worker.Stop(); });

    std::size_t release_count = 0;
    for (const bool preserve_history : {false, true, false}) {
        worker.Start();
        ASSERT_TRUE(work.WaitForLocked(true, utest::kMaxTestWaitTime));
        if (preserve_history) {
            settings.fallback_policy.emplace();
        } else {
            settings.fallback_policy.reset();
        }
        worker.UpdateSettings(settings);
        worker.Stop();

        const auto releases = strategy->GetReleases();
        ASSERT_EQ(releases.size(), ++release_count);
        EXPECT_EQ(releases.back().preserve_history, preserve_history);
    }
    EXPECT_EQ(strategy->GetRequestsCount(), 0);
}

UTEST_MT(LockedTask, NoWaitStopsWhenFallbackCheckFindsBusyLock, 3) {
    auto strategy = std::make_shared<ScriptedFallbackStrategy>();
    std::size_t work_count = 0;
    dist_lock::DistLockedTask task(
        kWorkerName,
        [&] { ++work_count; },
        strategy,
        MakeFallbackSettings(),
        dist_lock::DistLockWaitingMode::kNoWait
    );

    const auto check = strategy->WaitForRequest();
    ASSERT_TRUE(check.has_value());
    EXPECT_EQ(check->operation, dist_lock::LockOperation::kCheck);
    strategy->Reply(dist_lock::LockOperationResult::kBusy);

    task.WaitFor(utest::kMaxTestWaitTime);
    ASSERT_TRUE(task.IsFinished());
    UEXPECT_NO_THROW(task.Get());
    EXPECT_EQ(strategy->GetRequestsCount(), 1);
    EXPECT_EQ(work_count, 0);
}

UTEST_MT(LockedWorker, DisablingDuringFinalHeartbeatPreventsFallbackAcquire, 3) {
    auto strategy = std::make_shared<ScriptedFallbackStrategy>();
    auto settings = MakeFallbackSettings();
    settings.fallback_policy.value().fallback_after_checks = 1;
    std::atomic<std::size_t> work_count{0};
    dist_lock::DistLockedWorker worker(kWorkerName, [&] { ++work_count; }, strategy, settings);
    auto run = utils::Async("fallback-disabled-during-check", [&] { worker.RunOnce(); });

    const auto check = strategy->WaitForRequest();
    ASSERT_TRUE(check.has_value());
    EXPECT_EQ(check->operation, dist_lock::LockOperation::kCheck);
    settings.is_enabled = false;
    worker.UpdateSettings(settings);
    strategy->Reply(dist_lock::LockOperationResult::kNotAlive);

    run.WaitFor(utest::kMaxTestWaitTime);
    ASSERT_TRUE(run.IsFinished());
    UEXPECT_NO_THROW(run.Get());
    EXPECT_EQ(strategy->GetRequestsCount(), 1);
    EXPECT_EQ(work_count, 0);
    EXPECT_TRUE(worker.IsCancelAdvised());
}

UTEST_MT(LockedWorker, FallbackRequiresFreshSuccessfulChecks, 3) {
    auto strategy = std::make_shared<ScriptedFallbackStrategy>();
    DistLockWorkload work;
    const auto settings = MakeFallbackSettings();
    dist_lock::DistLockedWorker worker(kWorkerName, [&] { work.Work(); }, strategy, settings);
    const utils::FastScopeGuard stop([&]() noexcept { worker.Stop(); });
    using dist_lock::LockOperation;
    using dist_lock::LockOperationResult;

    worker.Start();
    const std::optional<LockOperationResult> replies[]{
        LockOperationResult::kNotAlive,
        LockOperationResult::kBusy,
        LockOperationResult::kNotAlive,
        std::nullopt,
        LockOperationResult::kNotAlive,
        LockOperationResult::kNotAlive,
        LockOperationResult::kNotAlive,
    };
    std::size_t expected_failures = 0;
    for (const auto reply : replies) {
        const auto request = strategy->WaitForRequest();
        ASSERT_TRUE(request.has_value());
        EXPECT_EQ(request->operation, LockOperation::kCheck);
        EXPECT_TRUE(worker.IsCancelAdvised());
        EXPECT_FALSE(work.IsLocked());
        EXPECT_EQ(worker.GetStatistics().lock_failures.Load().value, expected_failures);
        if (!reply || reply.value() == LockOperationResult::kBusy) {
            ++expected_failures;
        }
        strategy->Reply(reply);
    }

    const auto rejected_acquire = strategy->WaitForRequest();
    ASSERT_TRUE(rejected_acquire.has_value());
    EXPECT_EQ(rejected_acquire->operation, LockOperation::kFallback);
    EXPECT_EQ(rejected_acquire->min_idle_time_milliseconds, 3 * settings.acquire_interval);
    EXPECT_FALSE(work.IsLocked());
    strategy->Reply(LockOperationResult::kBusy);

    for (std::size_t i = 0; i < settings.fallback_policy.value().fallback_after_checks; ++i) {
        const auto request = strategy->WaitForRequest();
        ASSERT_TRUE(request.has_value());
        EXPECT_EQ(request->operation, LockOperation::kCheck);
        EXPECT_TRUE(worker.IsCancelAdvised());
        strategy->Reply(LockOperationResult::kNotAlive);
    }
    const auto accepted_acquire = strategy->WaitForRequest();
    ASSERT_TRUE(accepted_acquire.has_value());
    EXPECT_EQ(accepted_acquire->operation, LockOperation::kFallback);
    EXPECT_FALSE(work.IsLocked());
    strategy->Reply(LockOperationResult::kAcquired);
    ASSERT_TRUE(work.WaitForLocked(true, utest::kMaxTestWaitTime));
    EXPECT_FALSE(worker.IsCancelAdvised());
    EXPECT_TRUE(worker.OwnsLock());
}

UTEST_MT(LockedWorker, FallbackGrantExpiresAndConfigRevokesIt, 3) {
    utils::datetime::MockNowSet(std::chrono::system_clock::now());
    const utils::FastScopeGuard unset_time([]() noexcept { utils::datetime::MockNowUnset(); });
    auto strategy = std::make_shared<ScriptedFallbackStrategy>();
    auto settings = MakeFallbackSettings();
    settings.fallback_policy = dist_lock::DistLockFallbackSettings{
        .fallback_after_checks = 1,
        .fallback_duration = std::chrono::seconds{60},
    };
    settings.lock_ttl = std::chrono::hours{1};
    DistLockWorkload work;
    dist_lock::DistLockedWorker worker(kWorkerName, [&] { work.Work(); }, strategy, settings);
    const utils::FastScopeGuard stop([&]() noexcept { worker.Stop(); });
    using dist_lock::LockOperation;
    using dist_lock::LockOperationResult;

    worker.Start();
    for (std::size_t grant = 0; grant < 2; ++grant) {
        const auto check = strategy->WaitForRequest();
        ASSERT_TRUE(check.has_value());
        EXPECT_EQ(check->operation, LockOperation::kCheck);
        EXPECT_TRUE(worker.IsCancelAdvised());
        strategy->Reply(LockOperationResult::kNotAlive);
        const auto acquire = strategy->WaitForRequest();
        ASSERT_TRUE(acquire.has_value());
        EXPECT_EQ(acquire->operation, LockOperation::kFallback);
        strategy->Reply(LockOperationResult::kAcquired);
        ASSERT_TRUE(work.WaitForLocked(true, utest::kMaxTestWaitTime));
        EXPECT_FALSE(worker.IsCancelAdvised());

        if (grant == 0) {
            worker.UpdateSettings(settings);
            EXPECT_FALSE(worker.IsCancelAdvised());
            settings.prolong_interval += kAttemptInterval;
            worker.UpdateSettings(settings);
            EXPECT_TRUE(worker.IsCancelAdvised());
            settings.is_enabled = false;
            worker.UpdateSettings(settings);
            EXPECT_TRUE(worker.IsCancelAdvised());
            work.SetWorkLoopOn(false);
            ASSERT_TRUE(work.WaitForLocked(false, utest::kMaxTestWaitTime));
            ASSERT_TRUE(strategy->WaitForReleases(1));
            settings.is_enabled = true;
            worker.UpdateSettings(settings);
            EXPECT_TRUE(worker.IsCancelAdvised());
            work.SetWorkLoopOn(true);
        }
    }

    const auto prolongs = strategy->GetProlongsCount();
    utils::datetime::MockSleep(std::chrono::seconds{30});
    ASSERT_TRUE(strategy->WaitForProlongs(prolongs + 1));
    utils::datetime::MockSleep(std::chrono::seconds{30} - std::chrono::milliseconds{1});
    EXPECT_FALSE(worker.IsCancelAdvised());
    utils::datetime::MockSleep(std::chrono::milliseconds{1});
    EXPECT_TRUE(worker.IsCancelAdvised());
}

USERVER_NAMESPACE_END
