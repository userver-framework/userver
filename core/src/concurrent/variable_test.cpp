#include <userver/concurrent/variable.hpp>
#include <userver/engine/shared_mutex.hpp>
#include <userver/utils/async.hpp>

#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

UTEST(ConcurrentVariable, Ctr) { concurrent::Variable<int> variable; }

UTEST(ConcurrentVariable, UniqueLock) {
  concurrent::Variable<int> variable{41};
  auto lock = variable.UniqueLock();
  *lock = 42;
  ASSERT_EQ(*lock, 42);
}

UTEST(ConcurrentVariable, UniqueLockTry) {
  concurrent::Variable<int> variable{41};
  auto lock = variable.UniqueLock(std::try_to_lock);
  ASSERT_TRUE(lock);
  **lock = 42;
  ASSERT_EQ(**lock, 42);

  auto lock2 = variable.UniqueLock(std::try_to_lock);
  ASSERT_FALSE(lock2);
}

UTEST(ConcurrentVariable, UniqueLockWaitTimeout) {
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
}

UTEST(ConcurrentVariable, UniqueLockTryConst) {
  const concurrent::Variable<int> variable{42};
  auto lock = variable.UniqueLock(std::try_to_lock);
  ASSERT_TRUE(lock);
  ASSERT_EQ(**lock, 42);

  auto lock2 = variable.UniqueLock(std::try_to_lock);
  ASSERT_FALSE(lock2);
}

UTEST(ConcurrentVariable, UniqueLockWaitTimeoutConst) {
  const concurrent::Variable<int> variable{42};
  auto lock = variable.UniqueLock(std::try_to_lock);
  ASSERT_TRUE(lock);
  ASSERT_EQ(**lock, 42);

  auto task = utils::Async("test", [&variable] {
    auto lock2 = variable.UniqueLock(std::chrono::milliseconds(1));
    ASSERT_FALSE(lock2);
  });

  task.Get();
}

UTEST(ConcurrentVariable, SampleConcurrentVariable) {
  /// [Sample concurrent::Variable usage]
  constexpr auto kTestString = "Test string";

  struct Data {
    int x = 0;
    std::string s{};
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
}

USERVER_NAMESPACE_END
