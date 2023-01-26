#include <userver/utest/utest.hpp>

#include <userver/engine/async.hpp>
#include <userver/engine/shared_mutex.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/utils/async.hpp>

USERVER_NAMESPACE_BEGIN

UTEST(SharedMutex, SharedLockUnlockDouble) {
  engine::SharedMutex mutex;
  mutex.lock_shared();
  mutex.unlock_shared();

  mutex.lock_shared();
  mutex.unlock_shared();
}

UTEST(SharedMutex, SharedLockParallel) {
  engine::SharedMutex mutex;
  std::atomic<int> count{0};
  std::atomic<bool> was{false};

  std::vector<engine::Task> tasks;
  tasks.reserve(2);
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
}

UTEST(SharedMutex, SharedAndUniqueLock) {
  engine::SharedMutex mutex;

  std::unique_lock<engine::SharedMutex> lock(mutex);
  auto reader = utils::Async(
      "", [&mutex] { std::shared_lock<engine::SharedMutex> lock(mutex); });

  reader.WaitFor(std::chrono::milliseconds(50));
  EXPECT_FALSE(reader.IsFinished());

  lock.unlock();

  reader.WaitFor(std::chrono::milliseconds(50));
  EXPECT_TRUE(reader.IsFinished());
  UEXPECT_NO_THROW(reader.Get());
}

UTEST(SharedMutex, UniqueAndSharedLock) {
  engine::SharedMutex mutex;

  std::shared_lock<engine::SharedMutex> lock(mutex);
  auto writer = utils::Async(
      "", [&mutex] { std::unique_lock<engine::SharedMutex> lock(mutex); });

  writer.WaitFor(std::chrono::milliseconds(50));
  EXPECT_FALSE(writer.IsFinished());

  lock.unlock();

  writer.WaitFor(std::chrono::milliseconds(50));
  EXPECT_TRUE(writer.IsFinished());
  UEXPECT_NO_THROW(writer.Get());
}

UTEST_MT(SharedMutex, WritersDontStarve, 2) {
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
  readers.reserve(10);
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
}

UTEST(SharedMutex, TryLock) {
  engine::SharedMutex mutex;

  for (int i = 0; i < 10; i++) {
    ASSERT_TRUE(mutex.try_lock());
    mutex.unlock();
  }

  {
    // mutex must be free of writers
    std::shared_lock<engine::SharedMutex> lock(mutex);
  }
}

UTEST(SharedMutex, TryLockFail) {
  engine::SharedMutex mutex;

  std::unique_lock<engine::SharedMutex> lock(mutex);
  auto task = utils::Async("", [&mutex] { return mutex.try_lock(); });
  EXPECT_FALSE(task.Get());
}

UTEST(SharedMutex, SampleSharedMutex) {
  /// [Sample engine::SharedMutex usage]

  constexpr auto kTestString = "123";

  engine::SharedMutex mutex;
  std::string data;
  {
    std::lock_guard<engine::SharedMutex> lock(mutex);
    // accessing the data under the mutex for writing
    data = kTestString;
  }

  {
    std::shared_lock<engine::SharedMutex> lock(mutex);
    // accessing the data under the mutex for reading,
    // data cannot be changed
    const auto& x = data;
    ASSERT_EQ(x, kTestString);
  }
  /// [Sample engine::SharedMutex usage]
}

USERVER_NAMESPACE_END
