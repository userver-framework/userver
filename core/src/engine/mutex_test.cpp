#include <gtest/gtest.h>

#include <userver/engine/async.hpp>
#include <userver/engine/mutex.hpp>
#include <userver/engine/shared_mutex.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/single_waiting_task_mutex.hpp>
#include <userver/engine/sleep.hpp>

#include <userver/utest/utest.hpp>

using namespace std::chrono_literals;

USERVER_NAMESPACE_BEGIN

template <class T>
struct Mutex : public ::testing::Test {};
TYPED_UTEST_SUITE_P(Mutex);

TYPED_UTEST_P(Mutex, LockUnlock) {
  TypeParam mutex;
  mutex.lock();
  mutex.unlock();
}

TYPED_UTEST_P(Mutex, LockUnlockDouble) {
  TypeParam mutex;
  mutex.lock();
  mutex.unlock();

  mutex.lock();
  mutex.unlock();
}

TYPED_UTEST_P(Mutex, WaitAndCancel) {
  TypeParam mutex;
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

TYPED_UTEST_P(Mutex, TryLock) {
  TypeParam mutex;

  EXPECT_TRUE(!!std::unique_lock<TypeParam>(mutex, std::try_to_lock));
  EXPECT_TRUE(
      !!std::unique_lock<TypeParam>(mutex, std::chrono::milliseconds(10)));
  EXPECT_TRUE(
      !!std::unique_lock<TypeParam>(mutex, std::chrono::system_clock::now()));

  std::unique_lock lock(mutex);
  EXPECT_FALSE(engine::AsyncNoSpan([&mutex] {
                 return !!std::unique_lock<TypeParam>(mutex, std::try_to_lock);
               }).Get());

  EXPECT_FALSE(engine::AsyncNoSpan([&mutex] {
                 return !!std::unique_lock<TypeParam>(
                     mutex, std::chrono::milliseconds(10));
               }).Get());
  EXPECT_FALSE(engine::AsyncNoSpan([&mutex] {
                 return !!std::unique_lock<TypeParam>(
                     mutex, std::chrono::system_clock::now());
               }).Get());

  auto long_waiter = engine::AsyncNoSpan([&mutex] {
    return !!std::unique_lock<TypeParam>(mutex, utest::kMaxTestWaitTime);
  });
  engine::Yield();
  EXPECT_FALSE(long_waiter.IsFinished());
  lock.unlock();
  EXPECT_TRUE(long_waiter.Get());
}

namespace {
constexpr size_t kThreads = 4;
}  // namespace

TYPED_UTEST_P_MT(Mutex, LockPassing, kThreads) {
  static constexpr auto kTestDuration = std::chrono::milliseconds{500};

  const auto test_deadline = engine::Deadline::FromDuration(kTestDuration);
  TypeParam mutex;

  const size_t worker_count =
      std::is_same_v<TypeParam, engine::SingleWaitingTaskMutex> ? 2 : 4;

  const auto work = [&mutex] {
    std::unique_lock lock(mutex, std::defer_lock);
    ASSERT_TRUE(lock.try_lock_for(utest::kMaxTestWaitTime));
  };

  while (!test_deadline.IsReached()) {
    std::vector<engine::TaskWithResult<void>> tasks;
    for (size_t i = 0; i < worker_count; ++i) {
      tasks.push_back(engine::AsyncNoSpan(work));
    }
    for (auto& task : tasks) task.Get();
  }
}

TYPED_UTEST_P_MT(Mutex, NotifyAndDeadlineRace, 2) {
  if constexpr (std::is_same_v<TypeParam, engine::SingleWaitingTaskMutex>) {
    return;
  }
  constexpr int kTestIterationsCount = 1000;
  constexpr auto kSmallWaitTime = 5us;

  for (int i = 0; i < kTestIterationsCount; ++i) {
    TypeParam mutex;
    std::unique_lock lock(mutex);

    engine::SingleConsumerEvent lock_acquired;

    auto deadline_task = engine::AsyncNoSpan([&] {
      if (mutex.try_lock_for(kSmallWaitTime)) {
        mutex.unlock();
        lock_acquired.Send();
      }
    });

    auto no_deadline_task = engine::AsyncNoSpan([&] {
      if (mutex.try_lock_until(engine::Deadline{})) {
        mutex.unlock();
        lock_acquired.Send();
      }
    });

    engine::SleepFor(kSmallWaitTime);

    // After this, if 'deadline_task' has not timed out yet, it should acquire
    // the lock. If 'deadline_task' has timed out, 'no_deadline_task' should
    // acquire the lock.
    //
    // A bug could happen if we wake up 'deadline_task' while it's cancelling
    // itself due to a deadline. 'deadline_task' will wake up, but not lock
    // the mutex.
    lock.unlock();

    ASSERT_TRUE(lock_acquired.WaitForEventFor(utest::kMaxTestWaitTime));
  }
}

UTEST(Mutex, SampleMutex) {
  /// [Sample engine::Mutex usage]
  engine::Mutex mutex;
  constexpr std::string_view kTestData = "Test Data";

  {
    std::lock_guard<engine::Mutex> lock(mutex);
    // accessing data under a mutex
    const auto x = kTestData;
    ASSERT_EQ(kTestData, x);
  }
  /// [Sample engine::Mutex usage]
}

REGISTER_TYPED_UTEST_SUITE_P(Mutex,

                             LockUnlock, LockUnlockDouble, WaitAndCancel,
                             TryLock, LockPassing, NotifyAndDeadlineRace);

INSTANTIATE_TYPED_UTEST_SUITE_P(EngineMutex, Mutex, engine::Mutex);
INSTANTIATE_TYPED_UTEST_SUITE_P(EngineSharedMutex, Mutex, engine::SharedMutex);
INSTANTIATE_TYPED_UTEST_SUITE_P(EngineSingleWaitingTaskMutex, Mutex,
                                engine::SingleWaitingTaskMutex);

USERVER_NAMESPACE_END
