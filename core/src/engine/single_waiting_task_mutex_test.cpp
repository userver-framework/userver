#include <gtest/gtest.h>

#include <userver/engine/async.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/single_waiting_task_mutex.hpp>
#include <userver/engine/sleep.hpp>

#include <userver/utest/utest.hpp>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

UTEST(SingleWaitingTaskMutex, LockUnlock) {
  engine::SingleWaitingTaskMutex mutex;
  mutex.lock();
  mutex.unlock();
}

UTEST(SingleWaitingTaskMutex, LockUnlockDouble) {
  engine::SingleWaitingTaskMutex mutex;
  mutex.lock();
  mutex.unlock();

  mutex.lock();
  mutex.unlock();
}

UTEST(SingleWaitingTaskMutex, WaitAndCancel) {
  engine::SingleWaitingTaskMutex mutex;
  std::unique_lock lock(mutex);
  auto task = engine::AsyncNoSpan([&mutex]() { std::lock_guard lock(mutex); });

  task.WaitFor(std::chrono::milliseconds(50));
  EXPECT_FALSE(task.IsFinished());

  task.RequestCancel();
  task.WaitFor(std::chrono::milliseconds(50));
  EXPECT_FALSE(task.IsFinished());

  lock.unlock();
  task.WaitFor(std::chrono::milliseconds(50));
  EXPECT_TRUE(task.IsFinished());
  UEXPECT_NO_THROW(task.Get());
}

UTEST(Mutex, TryLock) {
  engine::SingleWaitingTaskMutex mutex;

  EXPECT_TRUE(
      !!std::unique_lock<engine::SingleWaitingTaskMutex>(mutex, std::try_to_lock));
  EXPECT_TRUE(!!std::unique_lock<engine::SingleWaitingTaskMutex>(
      mutex, std::chrono::milliseconds(10)));
  EXPECT_TRUE(!!std::unique_lock<engine::SingleWaitingTaskMutex>(
      mutex, std::chrono::system_clock::now()));

  std::unique_lock lock(mutex);
  EXPECT_FALSE(engine::AsyncNoSpan([&mutex] {
                 return !!std::unique_lock<engine::SingleWaitingTaskMutex>(
                     mutex, std::try_to_lock);
               }).Get());

  EXPECT_FALSE(engine::AsyncNoSpan([&mutex] {
                 return !!std::unique_lock<engine::SingleWaitingTaskMutex>(
                     mutex, std::chrono::milliseconds(10));
               }).Get());
  EXPECT_FALSE(engine::AsyncNoSpan([&mutex] {
                 return !!std::unique_lock<engine::SingleWaitingTaskMutex>(
                     mutex, std::chrono::system_clock::now());
               }).Get());

  auto long_waiter = engine::AsyncNoSpan([&mutex] {
    return !!std::unique_lock<engine::SingleWaitingTaskMutex>(
        mutex, utest::kMaxTestWaitTime);
  });
  engine::Yield();
  EXPECT_FALSE(long_waiter.IsFinished());
  lock.unlock();
  EXPECT_TRUE(long_waiter.Get());
}

namespace {
constexpr size_t kThreads = 2;
}  // namespace

UTEST_MT(SingleWaitingTaskMutex, LockPassing, kThreads) {
  static constexpr auto kTestDuration = std::chrono::milliseconds{500};

  const auto test_deadline = engine::Deadline::FromDuration(kTestDuration);
  engine::SingleWaitingTaskMutex mutex;

  const auto work = [&mutex] {
    std::unique_lock lock(mutex, std::defer_lock);
    ASSERT_TRUE(lock.try_lock_for(utest::kMaxTestWaitTime));
  };

  while (!test_deadline.IsReached()) {
    std::vector<engine::TaskWithResult<void>> tasks;
    for (size_t i = 0; i < kThreads; ++i) {
      tasks.push_back(engine::AsyncNoSpan(work));
    }
    for (auto& task : tasks) task.Get();
  }
}

UTEST(SingleWaitingTaskMutex, SampleMutex) {
  /// [Sample engine::Mutex usage]
  engine::SingleWaitingTaskMutex mutex;
  constexpr std::string_view kTestData = "Test Data";

  {
    std::lock_guard<engine::SingleWaitingTaskMutex> lock(mutex);
    // accessing data under a mutex
    const auto x = kTestData;
    ASSERT_EQ(kTestData, x);
  }
  /// [Sample engine::Mutex usage]
}

USERVER_NAMESPACE_END
