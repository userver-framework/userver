#include <userver/utest/utest.hpp>

#include <storages/mongo/util_mongotest.hpp>
#include <userver/formats/bson.hpp>
#include <userver/storages/mongo/collection.hpp>
#include <userver/storages/mongo/exception.hpp>
#include <userver/storages/mongo/pool.hpp>

using namespace formats::bson;
using namespace storages::mongo;

namespace {
Pool MakeTestPool() { return MakeTestsuiteMongoPool("exception_test"); }
}  // namespace

UTEST(Exception, DuplicateKey) {
  auto pool = MakeTestPool();
  auto coll = pool.GetCollection("duplicate_key");

  ASSERT_NO_THROW(coll.InsertOne(MakeDoc("_id", 1)));
  EXPECT_THROW(coll.InsertOne(MakeDoc("_id", 1)), DuplicateKeyException);
}
