#include <gtest/gtest.h>

#include <engine/async.hpp>
#include <engine/mutex.hpp>
#include <engine/shared_mutex.hpp>
#include <engine/sleep.hpp>

#include <utest/utest.hpp>

template <class T>
struct Mutex : public ::testing::Test {};
TYPED_TEST_SUITE_P(Mutex);

TYPED_TEST_P(Mutex, LockUnlock) {
  RunInCoro([] {
    TypeParam mutex;
    mutex.lock();
    mutex.unlock();
  });
}

TYPED_TEST_P(Mutex, LockUnlockDouble) {
  RunInCoro([] {
    TypeParam mutex;
    mutex.lock();
    mutex.unlock();

    mutex.lock();
    mutex.unlock();
  });
}

TYPED_TEST_P(Mutex, WaitAndCancel) {
  RunInCoro([] {
    TypeParam mutex;
    std::unique_lock lock(mutex);
    auto task =
        engine::impl::Async([&mutex]() { std::lock_guard lock(mutex); });

    task.WaitFor(std::chrono::milliseconds(50));
    EXPECT_FALSE(task.IsFinished());

    task.RequestCancel();
    task.WaitFor(std::chrono::milliseconds(50));
    EXPECT_FALSE(task.IsFinished());

    lock.unlock();
    task.WaitFor(std::chrono::milliseconds(50));
    EXPECT_TRUE(task.IsFinished());
    EXPECT_NO_THROW(task.Get());
  });
}

TYPED_TEST_P(Mutex, TryLock) {
  RunInCoro([] {
    TypeParam mutex;

    EXPECT_TRUE(!!std::unique_lock<TypeParam>(mutex, std::try_to_lock));
    EXPECT_TRUE(
        !!std::unique_lock<TypeParam>(mutex, std::chrono::milliseconds(10)));
    EXPECT_TRUE(
        !!std::unique_lock<TypeParam>(mutex, std::chrono::system_clock::now()));

    std::unique_lock lock(mutex);
    EXPECT_FALSE(engine::impl::Async([&mutex] {
                   return !!std::unique_lock<TypeParam>(mutex,
                                                        std::try_to_lock);
                 }).Get());

    EXPECT_FALSE(engine::impl::Async([&mutex] {
                   return !!std::unique_lock<TypeParam>(
                       mutex, std::chrono::milliseconds(10));
                 }).Get());
    EXPECT_FALSE(engine::impl::Async([&mutex] {
                   return !!std::unique_lock<TypeParam>(
                       mutex, std::chrono::system_clock::now());
                 }).Get());

    auto long_waiter = engine::impl::Async([&mutex] {
      return !!std::unique_lock<TypeParam>(mutex, std::chrono::seconds(10));
    });
    engine::Yield();
    EXPECT_FALSE(long_waiter.IsFinished());
    lock.unlock();
    EXPECT_TRUE(long_waiter.Get());
  });
}

TYPED_TEST_P(Mutex, LockPassing) {
  static constexpr size_t kThreads = 4;
  static constexpr auto kTestDuration = std::chrono::milliseconds{500};

  RunInCoro(
      [] {
        const auto test_deadline =
            engine::Deadline::FromDuration(kTestDuration);
        TypeParam mutex;

        const auto work = [&mutex] {
          std::unique_lock lock(mutex, std::defer_lock);
          ASSERT_TRUE(lock.try_lock_for(kMaxTestWaitTime));
        };

        while (!test_deadline.IsReached()) {
          std::vector<engine::TaskWithResult<void>> tasks;
          for (size_t i = 0; i < kThreads; ++i) {
            tasks.push_back(engine::impl::Async(work));
          }
          for (auto& task : tasks) task.Get();
        }
      },
      /*threads =*/kThreads);
}

TEST(Mutex, SampleMutex) {
  RunInCoro([] {
    /// [Sample engine::Mutex usage]
    ::engine::Mutex mutex;
    constexpr auto kTestData = "Test Data";

    {
      std::lock_guard<engine::Mutex> lock(mutex);
      // accessing data under a mutex
      auto x = kTestData;
      ASSERT_EQ(kTestData, x);
    }
    /// [Sample engine::Mutex usage]
  });
}

REGISTER_TYPED_TEST_SUITE_P(Mutex,

                            LockUnlock, LockUnlockDouble, WaitAndCancel,
                            TryLock, LockPassing);

INSTANTIATE_TYPED_TEST_SUITE_P(EngineMutex, Mutex, ::engine::Mutex);
INSTANTIATE_TYPED_TEST_SUITE_P(EngineSharedMutex, Mutex, ::engine::SharedMutex);
