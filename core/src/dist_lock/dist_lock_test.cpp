#include <utest/utest.hpp>

#include <dist_lock/dist_lock_settings.hpp>
#include <dist_lock/dist_lock_strategy.hpp>
#include <dist_lock/dist_locked_task.hpp>
#include <dist_lock/dist_locked_worker.hpp>
#include <engine/condition_variable.hpp>
#include <engine/mutex.hpp>
#include <engine/sleep.hpp>
#include <engine/task/cancel.hpp>
#include <logging/log.hpp>
#include <utils/async.hpp>
#include <utils/datetime.hpp>
#include <utils/mock_now.hpp>

namespace {

constexpr std::chrono::milliseconds kAttemptInterval{10};
constexpr std::chrono::milliseconds kLockTtl{100};
constexpr auto kAttemptTimeout = 5 * kAttemptInterval;

const std::string kWorkerName = "test";

dist_lock::DistLockSettings MakeSettings() {
  return {kAttemptInterval, kAttemptInterval, kLockTtl, kAttemptInterval,
          kAttemptInterval};
}

class MockDistLockStrategy : public dist_lock::DistLockStrategyBase {
 public:
  void Acquire(std::chrono::milliseconds) override {
    attempts_++;
    if (locked_) throw dist_lock::LockIsAcquiredByAnotherHostException();
    if (!allowed_) throw std::runtime_error("not allowed");
  }

  void Release() override {}

  void Allow(bool allowed) { allowed_ = allowed; }

  void LockedByOther(bool locked) { locked_ = locked; }

  size_t GetAttemptsCount() const { return attempts_; }

 private:
  std::atomic<bool> locked_{false};
  std::atomic<bool> allowed_{false};
  std::atomic<size_t> attempts_{0};
};

auto MakeMockStrategy() { return std::make_shared<MockDistLockStrategy>(); }

class DistLockWorkload {
 public:
  explicit DistLockWorkload(bool abort_on_cancel = false)
      : abort_on_cancel_(abort_on_cancel) {}

  bool IsLocked() const { return is_locked_; }

  bool WaitForLocked(bool locked, std::chrono::milliseconds ms) {
    std::unique_lock<engine::Mutex> lock(mutex_);
    return cv_.WaitFor(lock, ms,
                       [locked, this] { return locked == IsLocked(); });
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
      bool work_loop_on;
      while ((work_loop_on = work_loop_on_) &&
             !engine::current_task::IsCancelRequested()) {
        LOG_DEBUG() << "work loop";
        engine::InterruptibleSleepFor(std::chrono::milliseconds(50));
      }

      if (work_loop_on && abort_on_cancel_)
        engine::current_task::CancellationPoint();
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
    std::unique_lock<engine::Mutex> lock(mutex_);
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

TEST(LockedWorker, Noop) {
  RunInCoro([] {
    dist_lock::DistLockedWorker locked_worker(
        kWorkerName, [] {}, MakeMockStrategy(), MakeSettings());
  });
}

TEST(LockedWorker, StartStop) {
  RunInCoro(
      [] {
        auto strategy = MakeMockStrategy();
        DistLockWorkload work;
        dist_lock::DistLockedWorker locked_worker(
            kWorkerName, [&] { work.Work(); }, strategy, MakeSettings());
        EXPECT_FALSE(work.IsLocked());

        locked_worker.Start();
        EXPECT_FALSE(work.WaitForLocked(true, kAttemptTimeout));

        strategy->Allow(true);
        EXPECT_TRUE(work.WaitForLocked(true, kMaxTestWaitTime));

        locked_worker.Stop();
      },
      3);
}

TEST(LockedWorker, Watchdog) {
  RunInCoro(
      [] {
        auto strategy = MakeMockStrategy();
        DistLockWorkload work;
        dist_lock::DistLockedWorker locked_worker(
            kWorkerName, [&] { work.Work(); }, strategy, MakeSettings());

        locked_worker.Start();
        strategy->Allow(true);
        EXPECT_TRUE(work.WaitForLocked(true, kMaxTestWaitTime));

        strategy->Allow(false);
        EXPECT_TRUE(work.WaitForLocked(false, kMaxTestWaitTime));

        locked_worker.Stop();
      },
      3);
}

TEST(LockedWorker, OkAfterFail) {
  RunInCoro(
      [] {
        auto strategy = MakeMockStrategy();
        DistLockWorkload work;
        dist_lock::DistLockedWorker locked_worker(
            kWorkerName, [&] { work.Work(); }, strategy, MakeSettings());

        locked_worker.Start();
        EXPECT_FALSE(work.WaitForLocked(true, kAttemptTimeout));
        auto fail_count = strategy->GetAttemptsCount();
        EXPECT_LT(0, fail_count);
        EXPECT_FALSE(work.IsLocked());

        strategy->Allow(true);
        EXPECT_TRUE(work.WaitForLocked(true, kMaxTestWaitTime));
        EXPECT_LT(fail_count, strategy->GetAttemptsCount());

        locked_worker.Stop();
      },
      3);
}

// TODO: TAXICOMMON-1059
TEST(LockedWorker, DISABLED_IN_MAC_OS_TEST_NAME(OkFailOk)) {
  RunInCoro(
      [] {
        auto strategy = MakeMockStrategy();
        DistLockWorkload work;
        dist_lock::DistLockedWorker locked_worker(
            kWorkerName, [&] { work.Work(); }, strategy, MakeSettings());

        locked_worker.Start();
        strategy->Allow(true);
        EXPECT_TRUE(work.WaitForLocked(true, kMaxTestWaitTime));

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
      },
      3);
}

TEST(LockedWorker, LockedByOther) {
  RunInCoro(
      [] {
        auto strategy = MakeMockStrategy();
        DistLockWorkload work;
        dist_lock::DistLockedWorker locked_worker(
            kWorkerName, [&] { work.Work(); }, strategy, MakeSettings());

        locked_worker.Start();
        strategy->Allow(true);
        EXPECT_TRUE(work.WaitForLocked(true, kMaxTestWaitTime));

        strategy->LockedByOther(true);
        EXPECT_TRUE(work.WaitForLocked(false, kMaxTestWaitTime));

        strategy->LockedByOther(false);
        EXPECT_TRUE(work.WaitForLocked(false, kMaxTestWaitTime));

        locked_worker.Stop();
      },
      3);
}

TEST(LockedTask, Smoke) {
  RunInCoro(
      [] {
        auto strategy = MakeMockStrategy();
        DistLockWorkload work;
        dist_lock::DistLockedTask locked_task(kWorkerName, [&] { work.Work(); },
                                              strategy, MakeSettings());

        EXPECT_EQ(0u, work.GetFinishedWorkCount());
        strategy->Allow(true);
        EXPECT_TRUE(work.WaitForLocked(true, kAttemptTimeout));

        work.SetWorkLoopOn(false);
        strategy->Allow(false);
        locked_task.WaitFor(kMaxTestWaitTime);
        EXPECT_TRUE(locked_task.GetState() == engine::Task::State::kCompleted);
        EXPECT_EQ(1u, work.GetFinishedWorkCount());
      },
      3);
}

TEST(LockedTask, Fail) {
  RunInCoro(
      [] {
        auto settings = MakeSettings();
        settings.prolong_interval += settings.lock_ttl;  // make watchdog fire

        auto strategy = MakeMockStrategy();
        DistLockWorkload work(true);
        dist_lock::DistLockedTask locked_task(kWorkerName, [&] { work.Work(); },
                                              strategy, settings);

        EXPECT_EQ(0u, work.GetStartedWorkCount());
        EXPECT_EQ(0u, work.GetFinishedWorkCount());
        strategy->Allow(true);

        EXPECT_TRUE(work.WaitForLocked(true, kAttemptTimeout));
        locked_task.WaitFor(settings.prolong_interval + kAttemptTimeout);
        EXPECT_FALSE(locked_task.IsFinished());
        EXPECT_TRUE(work.WaitForLocked(true, kAttemptTimeout));
        EXPECT_TRUE(work.WaitForLocked(false, kMaxTestWaitTime));

        EXPECT_LE(1u, work.GetStartedWorkCount());
        EXPECT_EQ(0u, work.GetFinishedWorkCount());
      },
      3);
}

TEST(LockedTask, NoWait) {
  RunInCoro(
      [] {
        auto settings = MakeSettings();

        auto strategy = MakeMockStrategy();
        strategy->LockedByOther(true);

        DistLockWorkload work(true);
        dist_lock::DistLockedTask locked_task(
            kWorkerName, [&] { work.Work(); }, strategy, settings,
            dist_lock::DistLockWaitingMode::kNoWait);

        engine::InterruptibleSleepFor(3 * settings.prolong_interval);

        EXPECT_EQ(1, strategy->GetAttemptsCount());

        EXPECT_TRUE(locked_task.IsFinished());
        EXPECT_EQ(0u, work.GetStartedWorkCount());
        EXPECT_EQ(0u, work.GetFinishedWorkCount());
      },
      3);
}

TEST(LockedTask, NoWaitAquire) {
  RunInCoro(
      [] {
        auto strategy = MakeMockStrategy();
        DistLockWorkload work;

        EXPECT_EQ(0u, work.GetFinishedWorkCount());
        strategy->Allow(true);

        dist_lock::DistLockedTask locked_task(
            kWorkerName, [&] { work.Work(); }, strategy, MakeSettings(),
            dist_lock::DistLockWaitingMode::kNoWait);

        EXPECT_TRUE(work.WaitForLocked(true, kAttemptTimeout));

        work.SetWorkLoopOn(false);
        locked_task.WaitFor(kMaxTestWaitTime);

        EXPECT_TRUE(locked_task.GetState() == engine::Task::State::kCompleted);
        EXPECT_EQ(1u, work.GetFinishedWorkCount());
      },
      3);
}
