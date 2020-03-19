#include <concurrent/variable.hpp>
#include <utils/async.hpp>

#include <utest/utest.hpp>

TEST(ConcurrentVariable, Ctr) {
  RunInCoro([] { concurrent::Variable<int> variable; });
}

TEST(ConcurrentVariable, UniqueLock) {
  RunInCoro([] {
    concurrent::Variable<int> variable{42};
    auto lock = variable.UniqueLock();
    ASSERT_EQ(*lock, 42);
  });
}

TEST(ConcurrentVariable, UniqueLockTry) {
  RunInCoro([] {
    concurrent::Variable<int> variable{42};
    auto lock = variable.UniqueLock(std::try_to_lock);
    ASSERT_TRUE(lock);
    ASSERT_EQ(**lock, 42);

    auto lock2 = variable.UniqueLock(std::try_to_lock);
    ASSERT_FALSE(lock2);
  });
}

TEST(ConcurrentVariable, UniqueLockWaitTimeout) {
  RunInCoro([] {
    concurrent::Variable<int> variable{42};
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
