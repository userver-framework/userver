#include <gtest/gtest.h>

#include <engine/async.hpp>
#include <engine/mutex.hpp>
#include <engine/sleep.hpp>

#include <utest/utest.hpp>

TEST(Mutex, LockUnlock) {
  RunInCoro([] {
    engine::Mutex mutex;
    mutex.lock();
    mutex.unlock();
  });
}

TEST(Mutex, LockUnlockDouble) {
  RunInCoro([] {
    engine::Mutex mutex;
    mutex.lock();
    mutex.unlock();

    mutex.lock();
    mutex.unlock();
  });
}

TEST(Mutex, WaitAndCancel) {
  RunInCoro([] {
    engine::Mutex mutex;
    std::unique_lock<engine::Mutex> lock(mutex);
    auto task = engine::impl::Async(
        [&mutex]() { std::lock_guard<engine::Mutex> lock(mutex); });

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

TEST(Mutex, TryLock) {
  RunInCoro([] {
    engine::Mutex mutex;

    EXPECT_TRUE(!!std::unique_lock<engine::Mutex>(mutex, std::try_to_lock));
    EXPECT_TRUE(!!std::unique_lock<engine::Mutex>(
        mutex, std::chrono::milliseconds(10)));
    EXPECT_TRUE(!!std::unique_lock<engine::Mutex>(
        mutex, std::chrono::system_clock::now()));

    std::unique_lock<engine::Mutex> lock(mutex);
    EXPECT_FALSE(engine::impl::Async([&mutex] {
                   return !!std::unique_lock<engine::Mutex>(mutex,
                                                            std::try_to_lock);
                 })
                     .Get());

    EXPECT_FALSE(engine::impl::Async([&mutex] {
                   return !!std::unique_lock<engine::Mutex>(
                       mutex, std::chrono::milliseconds(10));
                 })
                     .Get());
    EXPECT_FALSE(engine::impl::Async([&mutex] {
                   return !!std::unique_lock<engine::Mutex>(
                       mutex, std::chrono::system_clock::now());
                 })
                     .Get());

    auto long_waiter = engine::impl::Async([&mutex] {
      return !!std::unique_lock<engine::Mutex>(mutex, std::chrono::seconds(10));
    });
    engine::Yield();
    EXPECT_FALSE(long_waiter.IsFinished());
    lock.unlock();
    EXPECT_TRUE(long_waiter.Get());
  });
}
