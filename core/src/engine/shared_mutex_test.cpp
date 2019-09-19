#include <utest/utest.hpp>

#include <engine/async.hpp>
#include <engine/shared_mutex.hpp>
#include <engine/sleep.hpp>
#include <utils/async.hpp>

TEST(SharedMutex, SharedLockUnlockDouble) {
  RunInCoro([] {
    engine::SharedMutex mutex;
    mutex.lock_shared();
    mutex.unlock_shared();

    mutex.lock_shared();
    mutex.unlock_shared();
  });
}

TEST(SharedMutex, SharedLockParallel) {
  RunInCoro([] {
    engine::SharedMutex mutex;
    std::atomic<int> count{0};
    std::atomic<bool> was{false};

    std::vector<engine::Task> tasks;
    for (auto i = 0; i < 2; i++)
      tasks.push_back(utils::Async("", [&mutex, &count, &was] {
        std::shared_lock<engine::SharedMutex> lock(mutex);

        count++;
        for (auto i = 0; i < 3 && count != 2; i++) {
          engine::Yield();
        }

        if (count == 2) was = true;
      }));

    for (auto i = 0; i < 20 && !was; i++) engine::Yield();

    EXPECT_TRUE(was.load());
  });
}

TEST(SharedMutex, SharedAndUniqueLock) {
  RunInCoro([] {
    engine::SharedMutex mutex;

    std::unique_lock<engine::SharedMutex> lock(mutex);
    auto reader = utils::Async(
        "", [&mutex] { std::shared_lock<engine::SharedMutex> lock(mutex); });

    reader.WaitFor(std::chrono::milliseconds(50));
    EXPECT_FALSE(reader.IsFinished());

    lock.unlock();

    reader.WaitFor(std::chrono::milliseconds(50));
    EXPECT_TRUE(reader.IsFinished());
    EXPECT_NO_THROW(reader.Get());
  });
}

TEST(SharedMutex, UniqueAndSharedLock) {
  RunInCoro([] {
    engine::SharedMutex mutex;

    std::shared_lock<engine::SharedMutex> lock(mutex);
    auto writer = utils::Async(
        "", [&mutex] { std::unique_lock<engine::SharedMutex> lock(mutex); });

    writer.WaitFor(std::chrono::milliseconds(50));
    EXPECT_FALSE(writer.IsFinished());

    lock.unlock();

    writer.WaitFor(std::chrono::milliseconds(50));
    EXPECT_TRUE(writer.IsFinished());
    EXPECT_NO_THROW(writer.Get());
  });
}

TEST(SharedMutex, WritersDontStarve) {
  RunInCoro(
      [] {
        engine::SharedMutex mutex;
        std::atomic<ssize_t> counter{0};
        std::atomic<ssize_t> loaded{-1};

        std::shared_lock<engine::SharedMutex> lock(mutex);
        auto writer = utils::Async("", [&mutex, &counter, &loaded] {
          std::unique_lock<engine::SharedMutex> lock(mutex);
          loaded = counter.load();
        });

        writer.WaitFor(std::chrono::milliseconds(50));
        EXPECT_FALSE(writer.IsFinished());

        std::vector<engine::Task> readers;
        for (int i = 0; i < 10; i++) {
          readers.push_back(utils::Async("", [&counter, &mutex] {
            std::shared_lock<engine::SharedMutex> lock(mutex);
            counter++;
          }));
        }

        writer.WaitFor(std::chrono::milliseconds(50));
        EXPECT_FALSE(writer.IsFinished());

        lock.unlock();

        writer.WaitFor(std::chrono::milliseconds(50));
        EXPECT_TRUE(writer.IsFinished());

        EXPECT_EQ(loaded.load(), 0);
      },
      2);
}

TEST(SharedMutex, TryLock) {
  RunInCoro([] {
    engine::SharedMutex mutex;

    for (int i = 0; i < 10; i++) {
      ASSERT_TRUE(mutex.try_lock());
      mutex.unlock();
    }

    {
      // mutex must be free of writers
      std::shared_lock<engine::SharedMutex> lock(mutex);
    }
  });
}

TEST(SharedMutex, TryLockFail) {
  RunInCoro([] {
    engine::SharedMutex mutex;

    std::unique_lock<engine::SharedMutex> lock(mutex);
    auto task = utils::Async("", [&mutex] { return mutex.try_lock(); });
    EXPECT_FALSE(task.Get());
  });
}
