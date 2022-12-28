#include <userver/utest/utest.hpp>

#include <chrono>
#include <string>
#include <vector>

#include <storages/mongo/util_mongotest.hpp>
#include <userver/clients/dns/resolver.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/formats/bson/document.hpp>
#include <userver/formats/bson/inline.hpp>
#include <userver/storages/mongo/collection.hpp>
#include <userver/storages/mongo/exception.hpp>
#include <userver/storages/mongo/pool.hpp>
#include <userver/storages/mongo/pool_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace mongo = storages::mongo;

namespace {
class Pool : public MongoPoolFixture {};
}  // namespace

UTEST_F(Pool, CollectionAccess) {
  static const std::string kSysVerCollName = "system.version";
  static const std::string kNonexistentCollName = "nonexistent";

  // this database always exists
  auto admin_pool = MakePool("admin", {});
  // this one should not exist yet
  auto test_pool = MakePool(kTestDatabaseDefaultName, {});

  EXPECT_TRUE(admin_pool.HasCollection(kSysVerCollName));
  UEXPECT_NO_THROW(admin_pool.GetCollection(kSysVerCollName));

  EXPECT_FALSE(test_pool.HasCollection(kSysVerCollName));
  UEXPECT_NO_THROW(test_pool.GetCollection(kSysVerCollName));

  EXPECT_FALSE(admin_pool.HasCollection(kNonexistentCollName));
  UEXPECT_NO_THROW(admin_pool.GetCollection(kNonexistentCollName));

  EXPECT_FALSE(test_pool.HasCollection(kNonexistentCollName));
  UEXPECT_NO_THROW(test_pool.GetCollection(kNonexistentCollName));
}

UTEST_F(Pool, DropDatabase) {
  static const std::string kCollName = "test";

  auto pool = GetDefaultPool();
  auto coll = pool.GetCollection(kCollName);

  UEXPECT_NO_THROW(coll.InsertOne(formats::bson::MakeDoc("_id", 42)));
  EXPECT_TRUE(pool.HasCollection(kCollName));

  UEXPECT_NO_THROW(pool.DropDatabase());
  EXPECT_FALSE(pool.HasCollection(kCollName));

  UEXPECT_NO_THROW(coll.InsertOne(formats::bson::MakeDoc("_id", 42)));
  EXPECT_TRUE(pool.HasCollection(kCollName));
}

UTEST(NonexistentPool, ConnectionFailure) {
  auto dns_resolver = MakeDnsResolver();
  auto dynamic_config = MakeDynamicConfig();

  // constructor should not throw
  mongo::Pool bad_pool("bad", "mongodb://%2Fnonexistent.sock/bad", {},
                       &dns_resolver, dynamic_config.GetSource());
  UEXPECT_THROW(bad_pool.HasCollection("test"),
                mongo::ClusterUnavailableException);
}

UTEST_F(Pool, Limits) {
  mongo::PoolConfig limited_config{};
  limited_config.initial_size = 1;
  limited_config.idle_limit = 1;
  limited_config.max_size = 1;
  auto limited_pool = MakePool({}, limited_config);

  std::vector<formats::bson::Document> docs;
  docs.reserve(150);
  /// large enough to not fit into a single batch
  for (int i = 0; i < 150; ++i) {
    docs.push_back(formats::bson::MakeDoc("_id", i));
  }
  limited_pool.GetCollection("test").InsertMany(std::move(docs));

  auto cursor = limited_pool.GetCollection("test").Find({});

  auto second_find = engine::AsyncNoSpan(
      [&limited_pool] { limited_pool.GetCollection("test").Find({}); });
  UEXPECT_THROW(second_find.Get(), mongo::MongoException);
}

USERVER_NAMESPACE_END
