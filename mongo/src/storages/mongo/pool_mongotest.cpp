#include <userver/utest/utest.hpp>

#include <chrono>
#include <string>
#include <vector>

#include <userver/engine/async.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/formats/bson/document.hpp>
#include <userver/formats/bson/inline.hpp>
#include <userver/storages/mongo/collection.hpp>
#include <userver/storages/mongo/exception.hpp>
#include <userver/storages/mongo/pool.hpp>
#include <userver/storages/mongo/pool_config.hpp>

using namespace storages::mongo;

namespace {
const PoolConfig kPoolConfig("userver_pool_test",
                             PoolConfig::DriverImpl::kMongoCDriver);
}  // namespace

UTEST(Pool, CollectionAccess) {
  // this database always exists
  Pool adminPool("admin", "mongodb://localhost:27217/admin", kPoolConfig);
  // this one should not exist
  Pool testPool("pool_test", "mongodb://localhost:27217/pool_test",
                kPoolConfig);

  static const std::string kSysVerCollName = "system.version";
  static const std::string kNonexistentCollName = "nonexistent";

  EXPECT_TRUE(adminPool.HasCollection(kSysVerCollName));
  EXPECT_NO_THROW(adminPool.GetCollection(kSysVerCollName));

  EXPECT_FALSE(testPool.HasCollection(kSysVerCollName));
  EXPECT_NO_THROW(testPool.GetCollection(kSysVerCollName));

  EXPECT_FALSE(adminPool.HasCollection(kNonexistentCollName));
  EXPECT_NO_THROW(adminPool.GetCollection(kNonexistentCollName));

  EXPECT_FALSE(testPool.HasCollection(kNonexistentCollName));
  EXPECT_NO_THROW(testPool.GetCollection(kNonexistentCollName));
}

UTEST(Pool, ConnectionFailure) {
  // constructor should not throw
  Pool badPool("bad", "mongodb://%2Fnonexistent.sock/bad", kPoolConfig);
  EXPECT_THROW(badPool.HasCollection("test"), ClusterUnavailableException);
}

UTEST(Pool, Limits) {
  PoolConfig limited_config = kPoolConfig;
  limited_config.max_size = 1;
  Pool limited_pool("limits_test", "mongodb://localhost:27217/limits_test",
                    limited_config);

  std::vector<formats::bson::Document> docs;
  /// large enough to not fit into a single batch
  for (int i = 0; i < 150; ++i) {
    docs.push_back(formats::bson::MakeDoc("_id", i));
  }
  limited_pool.GetCollection("test").InsertMany(std::move(docs));

  auto cursor = limited_pool.GetCollection("test").Find({});

  auto second_find = engine::impl::Async(
      [&limited_pool] { limited_pool.GetCollection("test").Find({}); });
  EXPECT_THROW(second_find.Get(), MongoException);
}
