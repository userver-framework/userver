#include <utest/utest.hpp>

#include <concurrent/mutex_set.hpp>
#include <engine/sleep.hpp>
#include <utils/async.hpp>

TEST(MutexSet, Ctr) {
  concurrent::MutexSet ms;
  concurrent::MutexSet<int> ms_int;
}

TEST(MutexSet, LockUnlock) {
  RunInCoro([] {
    concurrent::MutexSet ms;
    auto mutex = ms.GetMutexForKey("123");
    std::lock_guard lock(mutex);
  });
}

TEST(MutexSet, MultipleKeys) {
  RunInCoro([] {
    concurrent::MutexSet ms;
    auto m1 = ms.GetMutexForKey("123");
    auto m2 = ms.GetMutexForKey("1234");
    std::unique_lock lock1(m1);
    std::unique_lock lock2(m2);
  });
}

TEST(MutexSet, TryLock) {
  RunInCoro([] {
    concurrent::MutexSet ms;
    auto m1 = ms.GetMutexForKey("123");
    ASSERT_TRUE(m1.try_lock());
    EXPECT_FALSE(m1.try_lock());
    m1.unlock();
  });
}

TEST(MutexSet, Conflict) {
  RunInCoro([] {
    concurrent::MutexSet ms;
    auto m1 = ms.GetMutexForKey("123");
    auto m2 = ms.GetMutexForKey("123");

    ASSERT_TRUE(m1.try_lock());
    EXPECT_FALSE(m2.try_lock());

    m1.unlock();
    EXPECT_TRUE(m2.try_lock());
    m2.unlock();
  });
}

TEST(MutexSet, Notify) {
  RunInCoro([] {
    concurrent::MutexSet ms;
    auto m1 = ms.GetMutexForKey("123");
    std::unique_lock lock(m1);

    auto task = utils::Async("test", [&ms] {
      auto m2 = ms.GetMutexForKey("123");
      std::unique_lock lock(m2);
    });

    engine::Yield();
    engine::Yield();

    EXPECT_FALSE(task.IsFinished());

    lock.unlock();
    task.Get();
  });
}
