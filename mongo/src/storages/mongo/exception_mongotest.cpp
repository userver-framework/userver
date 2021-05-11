#include <utest/utest.hpp>

#include <formats/bson.hpp>
#include <storages/mongo/collection.hpp>
#include <storages/mongo/exception.hpp>
#include <storages/mongo/pool.hpp>
#include <storages/mongo/pool_config.hpp>

using namespace formats::bson;
using namespace storages::mongo;

namespace {
Pool MakeTestPool() {
  return {"exception_test", "mongodb://localhost:27217/exception_test",
          PoolConfig("userver_exception_test",
                     PoolConfig::DriverImpl::kMongoCDriver)};
}
}  // namespace

TEST(Exception, DuplicateKey) {
  RunInCoro([] {
    auto pool = MakeTestPool();
    auto coll = pool.GetCollection("duplicate_key");

    ASSERT_NO_THROW(coll.InsertOne(MakeDoc("_id", 1)));
    EXPECT_THROW(coll.InsertOne(MakeDoc("_id", 1)), DuplicateKeyException);
  });
}
