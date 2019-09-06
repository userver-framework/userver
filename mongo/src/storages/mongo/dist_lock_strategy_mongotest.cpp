#include <utest/utest.hpp>

#include <chrono>

#include <formats/bson.hpp>
#include <storages/mongo/dist_lock_strategy.hpp>
#include <storages/mongo/pool.hpp>
#include <storages/mongo/pool_config.hpp>
#include <utils/mock_now.hpp>

using namespace formats::bson;
using namespace storages::mongo;
using namespace std::chrono_literals;

namespace {

Pool MakeTestPool() {
  return {"collection_test", "mongodb://localhost:27217/collection_test",
          PoolConfig("userver_collection_test")};
}

Collection MakeCollection(const std::string& name) {
  auto collection = MakeTestPool().GetCollection(name);
  collection.DeleteMany({});
  return collection;
}

const auto kMockTime = std::chrono::system_clock::from_time_t(1567544400);

}  // namespace

TEST(DistLockTest, AcquireAndRelease) {
  utils::datetime::MockNowSet(kMockTime);

  RunInCoro([] {
    auto collection = MakeCollection("test_acquire_and_release");
    DistLockStrategy strategy(collection, "key_simple", "owner");
    EXPECT_NO_THROW(strategy.Acquire(1s));
    EXPECT_NO_THROW(strategy.Release());
  });
}

TEST(DistLockTest, Prolong) {
  utils::datetime::MockNowSet(kMockTime);

  RunInCoro([] {
    auto collection = MakeCollection("test_prolong");
    DistLockStrategy strategy(collection, "key_prolong", "owner");
    EXPECT_NO_THROW(strategy.Acquire(1s));
    EXPECT_NO_THROW(strategy.Acquire(1s));
  });
}

TEST(DistLockTest, TestOwner) {
  utils::datetime::MockNowSet(kMockTime);

  RunInCoro([] {
    auto collection = MakeCollection("test_owner");
    const std::string key = "key_task";
    DistLockStrategy strategy1(collection, key, "owner1");
    DistLockStrategy strategy2(collection, key, "owner2");

    EXPECT_NO_THROW(strategy1.Acquire(1s));
    ASSERT_THROW(strategy2.Acquire(1s),
                 dist_lock::LockIsAcquiredByAnotherHostException);
    EXPECT_NO_THROW(strategy2.Release());
  });
}

TEST(DistLockTest, Expire) {
  utils::datetime::MockNowSet(kMockTime);

  RunInCoro([] {
    auto collection = MakeCollection("test_expire");
    const std::string key = "key_expire";
    DistLockStrategy strategy1(collection, key, "owner1");
    DistLockStrategy strategy2(collection, key, "owner2");

    EXPECT_NO_THROW(strategy1.Acquire(1s));
    utils::datetime::MockSleep(5s);
    EXPECT_NO_THROW(strategy2.Acquire(1s));
    EXPECT_THROW(strategy1.Acquire(1s),
                 dist_lock::LockIsAcquiredByAnotherHostException);
  });
}

TEST(DistLockTest, ReleaseAcquire) {
  utils::datetime::MockNowSet(kMockTime);

  RunInCoro([] {
    auto collection = MakeCollection("test_release_acquire");
    const std::string key = "key_release_acquire";
    DistLockStrategy strategy1(collection, key, "owner1");
    DistLockStrategy strategy2(collection, key, "owner2");

    EXPECT_NO_THROW(strategy1.Acquire(1s));
    EXPECT_NO_THROW(strategy2.Release());
    EXPECT_NO_THROW(strategy1.Acquire(1s));
    EXPECT_NO_THROW(strategy1.Release());
    EXPECT_NO_THROW(strategy2.Acquire(1s));
  });
}
