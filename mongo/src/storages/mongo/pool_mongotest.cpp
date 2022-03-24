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

using namespace storages::mongo;

UTEST(Pool, CollectionAccess) {
  static const std::string kSysVerCollName = "system.version";
  static const std::string kNonexistentCollName = "nonexistent";

  auto dns_resolver = MakeDnsResolver();
  // this database always exists
  auto admin_pool = MakeTestsuiteMongoPool("admin", &dns_resolver);
  // this one should not exist
  auto test_pool = MakeTestsuiteMongoPool("pool_test", &dns_resolver);

  EXPECT_TRUE(admin_pool.HasCollection(kSysVerCollName));
  UEXPECT_NO_THROW(admin_pool.GetCollection(kSysVerCollName));

  EXPECT_FALSE(test_pool.HasCollection(kSysVerCollName));
  UEXPECT_NO_THROW(test_pool.GetCollection(kSysVerCollName));

  EXPECT_FALSE(admin_pool.HasCollection(kNonexistentCollName));
  UEXPECT_NO_THROW(admin_pool.GetCollection(kNonexistentCollName));

  EXPECT_FALSE(test_pool.HasCollection(kNonexistentCollName));
  UEXPECT_NO_THROW(test_pool.GetCollection(kNonexistentCollName));
}

UTEST(Pool, ConnectionFailure) {
  auto dns_resolver = MakeDnsResolver();

  // constructor should not throw
  Pool bad_pool("bad", "mongodb://%2Fnonexistent.sock/bad",
                {"bad", PoolConfig::DriverImpl::kMongoCDriver}, &dns_resolver,
                {});
  UEXPECT_THROW(bad_pool.HasCollection("test"), ClusterUnavailableException);
}

UTEST(Pool, Limits) {
  auto dns_resolver = MakeDnsResolver();
  PoolConfig limited_config{"limited", PoolConfig::DriverImpl::kMongoCDriver};
  limited_config.max_size = 1;
  auto limited_pool =
      MakeTestsuiteMongoPool("limits_test", limited_config, &dns_resolver);

  std::vector<formats::bson::Document> docs;
  /// large enough to not fit into a single batch
  for (int i = 0; i < 150; ++i) {
    docs.push_back(formats::bson::MakeDoc("_id", i));
  }
  limited_pool.GetCollection("test").InsertMany(std::move(docs));

  auto cursor = limited_pool.GetCollection("test").Find({});

  auto second_find = engine::AsyncNoSpan(
      [&limited_pool] { limited_pool.GetCollection("test").Find({}); });
  UEXPECT_THROW(second_find.Get(), MongoException);
}

USERVER_NAMESPACE_END
