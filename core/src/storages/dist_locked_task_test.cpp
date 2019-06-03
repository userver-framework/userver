#include <engine/condition_variable.hpp>
#include <engine/mutex.hpp>
#include <engine/sleep.hpp>
#include <engine/task/cancel.hpp>
#include <logging/log.hpp>
#include <storages/dist_locked_task.hpp>
#include <utest/utest.hpp>
#include <utils/mock_now.hpp>

namespace {

constexpr std::chrono::milliseconds kAttemptInterval{10};
constexpr auto kAttemptTimeout = 5 * kAttemptInterval;

storages::DistLockedTaskSettings CreateSettings() {
  using namespace std::chrono_literals;
  return storages::DistLockedTaskSettings{kAttemptInterval, kAttemptInterval,
                                          100ms, kAttemptInterval};
}
}  // namespace

class LockedTaskMock : public storages::DistLockedTask {
 public:
  LockedTaskMock(DistLockedTask::WorkerFunc func)
      : DistLockedTask("test", std::move(func), CreateSettings()) {}

  void Allow(bool allowed) { allowed_ = allowed; }

  void LockedByOther(bool locked) { locked_ = locked; }

  size_t GetAttemtpsCount() const { return attempts_; }

 protected:
  void RequestAcquire(std::chrono::milliseconds) override {
    attempts_++;
    if (locked_) throw DistLockedTask::LockIsAcquiredByAnotherHostError();
    if (!allowed_) throw std::runtime_error("not allowed");
  }

  void RequestRelease() override {}

 private:
  std::atomic<bool> locked_{false};
  std::atomic<bool> allowed_{false};
  std::atomic<size_t> attempts_{0};
};

class LockedTaskMockNoop : public LockedTaskMock {
 public:
  LockedTaskMockNoop() : LockedTaskMock([this] { Work(); }) {}

  bool IsLocked() const { return is_locked_; }

  bool WaitForLocked(bool locked, std::chrono::milliseconds ms) {
    std::unique_lock<engine::Mutex> lock(mutex_);
    return cv_.WaitFor(lock, ms,
                       [locked, this] { return locked == IsLocked(); });
  }

 private:
  void SetLocked(bool locked) {
    std::unique_lock<engine::Mutex> lock(mutex_);
    is_locked_ = locked;
    cv_.NotifyAll();
  }

  void Work() {
    LOG_DEBUG() << "work begin";
    SetLocked(true);
    LOG_DEBUG() << "work begin after SetLocked(true)";

    try {
      while (!engine::current_task::IsCancelRequested()) {
        LOG_DEBUG() << "work loop";
        engine::InterruptibleSleepFor(std::chrono::milliseconds(50));
      }
    } catch (...) {
      SetLocked(false);
      throw;
    }

    LOG_DEBUG() << "work end";
    SetLocked(false);
    LOG_DEBUG() << "work end after SetLocked(false)";
  }

  std::atomic<bool> is_locked_{false};
  engine::Mutex mutex_;
  engine::ConditionVariable cv_;
};

TEST(LockedTask, Noop) {
  RunInCoro([] { LockedTaskMockNoop locked_task; });
}

TEST(LockedTask, StartStop) {
  utils::datetime::MockNowUnset();

  RunInCoro(
      [] {
        LockedTaskMockNoop locked_task;
        EXPECT_FALSE(locked_task.IsLocked());

        locked_task.Start();
        EXPECT_FALSE(locked_task.WaitForLocked(true, kAttemptTimeout));

        locked_task.Allow(true);
        EXPECT_TRUE(locked_task.WaitForLocked(true, kMaxTestWaitTime));

        locked_task.Stop();
      },
      3);
}

TEST(LockedTask, Watchdog) {
  utils::datetime::MockNowUnset();

  RunInCoro(
      [] {
        LockedTaskMockNoop locked_task;

        locked_task.Start();
        locked_task.Allow(true);
        EXPECT_TRUE(locked_task.WaitForLocked(true, kMaxTestWaitTime));

        locked_task.Allow(false);
        EXPECT_TRUE(locked_task.WaitForLocked(false, kMaxTestWaitTime));

        locked_task.Stop();
      },
      3);
}

TEST(LockedTask, OkAfterFail) {
  utils::datetime::MockNowUnset();

  RunInCoro(
      [] {
        LockedTaskMockNoop locked_task;

        locked_task.Start();
        EXPECT_FALSE(locked_task.WaitForLocked(true, kAttemptTimeout));
        auto fail_count = locked_task.GetAttemtpsCount();
        EXPECT_LT(0, fail_count);
        EXPECT_FALSE(locked_task.IsLocked());

        locked_task.Allow(true);
        EXPECT_TRUE(locked_task.WaitForLocked(true, kMaxTestWaitTime));
        EXPECT_LT(fail_count, locked_task.GetAttemtpsCount());

        locked_task.Stop();
      },
      3);
}

// TODO: TAXICOMMON-1059
TEST(LockedTask, DISABLED_IN_MAC_OS_TEST_NAME(OkFailOk)) {
  utils::datetime::MockNowUnset();

  RunInCoro(
      [] {
        LockedTaskMockNoop locked_task;

        locked_task.Start();
        locked_task.Allow(true);
        EXPECT_TRUE(locked_task.WaitForLocked(true, kMaxTestWaitTime));

        locked_task.Allow(false);
        auto attempts_count = locked_task.GetAttemtpsCount();
        EXPECT_LT(0, attempts_count);
        EXPECT_FALSE(locked_task.WaitForLocked(false, kAttemptTimeout));

        auto attempts_count2 = locked_task.GetAttemtpsCount();
        EXPECT_LT(attempts_count, attempts_count2);

        locked_task.Allow(true);
        // FIXME
        EXPECT_FALSE(locked_task.WaitForLocked(false, kAttemptTimeout));
        auto attempts_count3 = locked_task.GetAttemtpsCount();
        EXPECT_LT(attempts_count2, attempts_count3);

        locked_task.Stop();
      },
      3);
}

TEST(LockedTask, LockedByOther) {
  utils::datetime::MockNowUnset();

  RunInCoro(
      [] {
        LockedTaskMockNoop locked_task;

        locked_task.Start();
        locked_task.Allow(true);
        EXPECT_TRUE(locked_task.WaitForLocked(true, kMaxTestWaitTime));

        locked_task.LockedByOther(true);
        EXPECT_TRUE(locked_task.WaitForLocked(false, kMaxTestWaitTime));

        locked_task.LockedByOther(false);
        EXPECT_TRUE(locked_task.WaitForLocked(false, kMaxTestWaitTime));

        locked_task.Stop();
      },
      3);
}
