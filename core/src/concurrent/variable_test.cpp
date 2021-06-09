#include <concurrent/variable.hpp>
#include <engine/shared_mutex.hpp>
#include <utils/async.hpp>

#include <utest/utest.hpp>

TEST(ConcurrentVariable, Ctr) {
  RunInCoro([] { concurrent::Variable<int> variable; });
}

TEST(ConcurrentVariable, UniqueLock) {
  RunInCoro([] {
    concurrent::Variable<int> variable{41};
    auto lock = variable.UniqueLock();
    *lock = 42;
    ASSERT_EQ(*lock, 42);
  });
}

TEST(ConcurrentVariable, UniqueLockTry) {
  RunInCoro([] {
    concurrent::Variable<int> variable{41};
    auto lock = variable.UniqueLock(std::try_to_lock);
    ASSERT_TRUE(lock);
    **lock = 42;
    ASSERT_EQ(**lock, 42);

    auto lock2 = variable.UniqueLock(std::try_to_lock);
    ASSERT_FALSE(lock2);
  });
}

TEST(ConcurrentVariable, UniqueLockWaitTimeout) {
  RunInCoro([] {
    concurrent::Variable<int> variable{41};
    auto lock = variable.UniqueLock(std::try_to_lock);
    ASSERT_TRUE(lock);
    **lock = 42;
    ASSERT_EQ(**lock, 42);

    auto task = utils::Async("test", [&variable] {
      auto lock2 = variable.UniqueLock(std::chrono::milliseconds(1));
      ASSERT_FALSE(lock2);
    });

    task.Get();
  });
}

TEST(ConcurrentVariable, UniqueLockTryConst) {
  RunInCoro([] {
    const concurrent::Variable<int> variable{42};
    auto lock = variable.UniqueLock(std::try_to_lock);
    ASSERT_TRUE(lock);
    ASSERT_EQ(**lock, 42);

    auto lock2 = variable.UniqueLock(std::try_to_lock);
    ASSERT_FALSE(lock2);
  });
}

TEST(ConcurrentVariable, UniqueLockWaitTimeoutConst) {
  RunInCoro([] {
    const concurrent::Variable<int> variable{42};
    auto lock = variable.UniqueLock(std::try_to_lock);
    ASSERT_TRUE(lock);
    ASSERT_EQ(**lock, 42);

    auto task = utils::Async("test", [&variable] {
      auto lock2 = variable.UniqueLock(std::chrono::milliseconds(1));
      ASSERT_FALSE(lock2);
    });

    task.Get();
  });
}

TEST(ConcurrentVariable, SampleConcurrentVariable) {
  RunInCoro([] {
    /// [Sample concurrent::Variable usage]
    constexpr auto kTestString = "Test string";

    struct Data {
      int x;
      std::string s;
    };
    // If you do not specify the 2nd template parameter,
    // then by default Variable is protected by engine::Mutex.
    concurrent::Variable<Data, engine::SharedMutex> data;

    {
      // We get a smart pointer to the data,
      // the smart pointer stores std::lock_guard
      auto data_ptr = data.Lock();
      data_ptr->s = kTestString;
    }

    {
      // We get a smart pointer to the data,
      // the smart pointer stores std::shared_lock
      auto data_ptr = data.SharedLock();
      // we can read data, we cannot write
      ASSERT_EQ(data_ptr->s, kTestString);
    }
    /// [Sample concurrent::Variable usage]
  });
}
