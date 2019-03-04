#include <utest/utest.hpp>

#include <formats/bson.hpp>
#include <storages/mongo_ng/collection.hpp>
#include <storages/mongo_ng/exception.hpp>
#include <storages/mongo_ng/pool.hpp>
#include <storages/mongo_ng/pool_config.hpp>

using namespace formats::bson;
using namespace storages::mongo_ng;

namespace {
Pool MakeTestPool() {
  return {"exception_test", "mongodb://localhost:27217/exception_test",
          PoolConfig("userver_exception_test")};
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
