#include <userver/utest/utest.hpp>

#include <chrono>

#include <storages/mongo/util_mongotest.hpp>
#include <userver/formats/bson.hpp>
#include <userver/storages/mongo/dist_lock_strategy.hpp>
#include <userver/storages/mongo/pool.hpp>
#include <userver/utils/mock_now.hpp>

using namespace formats::bson;
using namespace storages::mongo;
using namespace std::chrono_literals;

namespace {

Pool MakeTestPool() { return MakeTestsuiteMongoPool("collection_test"); }

Collection MakeCollection(const std::string& name) {
  auto collection = MakeTestPool().GetCollection(name);
  collection.DeleteMany({});
  return collection;
}

const auto kMockTime = std::chrono::system_clock::from_time_t(1567544400);

}  // namespace

UTEST(DistLockTest, AcquireAndRelease) {
  utils::datetime::MockNowSet(kMockTime);

  auto collection = MakeCollection("test_acquire_and_release");
  DistLockStrategy strategy(collection, "key_simple", "owner");
  EXPECT_NO_THROW(strategy.Acquire(1s, {}));
  EXPECT_NO_THROW(strategy.Release({}));
}

UTEST(DistLockTest, Prolong) {
  utils::datetime::MockNowSet(kMockTime);

  auto collection = MakeCollection("test_prolong");
  DistLockStrategy strategy(collection, "key_prolong", "owner");
  EXPECT_NO_THROW(strategy.Acquire(1s, {}));
  EXPECT_NO_THROW(strategy.Acquire(1s, {}));
}

UTEST(DistLockTest, TestOwner) {
  utils::datetime::MockNowSet(kMockTime);

  auto collection = MakeCollection("test_owner");
  const std::string key = "key_task";
  DistLockStrategy strategy1(collection, key, "owner1");
  DistLockStrategy strategy2(collection, key, "owner2");

  EXPECT_NO_THROW(strategy1.Acquire(1s, "first"));
  EXPECT_THROW(strategy1.Acquire(1s, "second"),
               dist_lock::LockIsAcquiredByAnotherHostException);
  ASSERT_THROW(strategy2.Acquire(1s, {}),
               dist_lock::LockIsAcquiredByAnotherHostException);
  EXPECT_NO_THROW(strategy2.Release("first"));
}

UTEST(DistLockTest, Expire) {
  utils::datetime::MockNowSet(kMockTime);

  auto collection = MakeCollection("test_expire");
  const std::string key = "key_expire";
  DistLockStrategy strategy1(collection, key, "owner1");
  DistLockStrategy strategy2(collection, key, "owner2");

  EXPECT_NO_THROW(strategy1.Acquire(1s, {}));
  utils::datetime::MockSleep(5s);
  EXPECT_NO_THROW(strategy2.Acquire(1s, {}));
  EXPECT_THROW(strategy1.Acquire(1s, {}),
               dist_lock::LockIsAcquiredByAnotherHostException);
}

UTEST(DistLockTest, ReleaseAcquire) {
  utils::datetime::MockNowSet(kMockTime);

  auto collection = MakeCollection("test_release_acquire");
  const std::string key = "key_release_acquire";
  DistLockStrategy strategy1(collection, key, "owner1");
  DistLockStrategy strategy2(collection, key, "owner2");

  EXPECT_NO_THROW(strategy1.Acquire(1s, {}));
  EXPECT_NO_THROW(strategy2.Release({}));
  EXPECT_NO_THROW(strategy1.Acquire(1s, {}));
  EXPECT_NO_THROW(strategy1.Release({}));
  EXPECT_NO_THROW(strategy2.Acquire(1s, {}));
}
