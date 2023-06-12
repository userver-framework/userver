#include <userver/utest/utest.hpp>

#include <chrono>

#include <storages/mongo/util_mongotest.hpp>
#include <userver/formats/bson.hpp>
#include <userver/storages/mongo/dist_lock_strategy.hpp>
#include <userver/storages/mongo/pool.hpp>
#include <userver/utils/mock_now.hpp>

USERVER_NAMESPACE_BEGIN

namespace mongo = storages::mongo;

using namespace std::chrono_literals;

namespace {

class DistLockTest : public MongoPoolFixture {};

const auto kMockTime = std::chrono::system_clock::from_time_t(1567544400);

}  // namespace

UTEST_F(DistLockTest, AcquireAndRelease) {
  utils::datetime::MockNowSet(kMockTime);

  auto collection = GetDefaultPool().GetCollection("test_acquire_and_release");
  mongo::DistLockStrategy strategy(collection, "key_simple", "owner");
  UEXPECT_NO_THROW(strategy.Acquire(1s, {}));
  UEXPECT_NO_THROW(strategy.Release({}));
}

UTEST_F(DistLockTest, Prolong) {
  utils::datetime::MockNowSet(kMockTime);

  auto collection = GetDefaultPool().GetCollection("test_prolong");
  mongo::DistLockStrategy strategy(collection, "key_prolong", "owner");
  UEXPECT_NO_THROW(strategy.Acquire(1s, {}));
  UEXPECT_NO_THROW(strategy.Acquire(1s, {}));
}

UTEST_F(DistLockTest, TestOwner) {
  utils::datetime::MockNowSet(kMockTime);

  auto collection = GetDefaultPool().GetCollection("test_owner");
  const std::string key = "key_task";
  mongo::DistLockStrategy strategy1(collection, key, "owner1");
  mongo::DistLockStrategy strategy2(collection, key, "owner2");

  UEXPECT_NO_THROW(strategy1.Acquire(1s, "first"));
  UEXPECT_THROW(strategy1.Acquire(1s, "second"),
                dist_lock::LockIsAcquiredByAnotherHostException);
  UASSERT_THROW(strategy2.Acquire(1s, {}),
                dist_lock::LockIsAcquiredByAnotherHostException);
  UEXPECT_NO_THROW(strategy2.Release("first"));
}

UTEST_F(DistLockTest, Expire) {
  utils::datetime::MockNowSet(kMockTime);

  auto collection = GetDefaultPool().GetCollection("test_expire");
  const std::string key = "key_expire";
  mongo::DistLockStrategy strategy1(collection, key, "owner1");
  mongo::DistLockStrategy strategy2(collection, key, "owner2");

  UEXPECT_NO_THROW(strategy1.Acquire(1s, {}));
  utils::datetime::MockSleep(5s);
  UEXPECT_NO_THROW(strategy2.Acquire(1s, {}));
  UEXPECT_THROW(strategy1.Acquire(1s, {}),
                dist_lock::LockIsAcquiredByAnotherHostException);
}

UTEST_F(DistLockTest, ReleaseAcquire) {
  utils::datetime::MockNowSet(kMockTime);

  auto collection = GetDefaultPool().GetCollection("test_release_acquire");
  const std::string key = "key_release_acquire";
  mongo::DistLockStrategy strategy1(collection, key, "owner1");
  mongo::DistLockStrategy strategy2(collection, key, "owner2");

  UEXPECT_NO_THROW(strategy1.Acquire(1s, {}));
  UEXPECT_NO_THROW(strategy2.Release({}));
  UEXPECT_NO_THROW(strategy1.Acquire(1s, {}));
  UEXPECT_NO_THROW(strategy1.Release({}));
  UEXPECT_NO_THROW(strategy2.Acquire(1s, {}));
}

USERVER_NAMESPACE_END
