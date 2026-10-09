#include <userver/utest/utest.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <storages/mongo/cdriver/pool_access.hpp>
#include <storages/mongo/cdriver/pool_impl.hpp>
#include <storages/mongo/features.hpp>
#ifdef USERVER_FEATURE_MONGO_EXPERIMENTAL
#include <storages/mongo/cdriver_experimental/pool_impl.hpp>
#endif
#include <storages/mongo/pool_impl.hpp>
#include <storages/mongo/util_mongotest.hpp>
#include <userver/clients/dns/resolver.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/engine/task/task_with_result.hpp>
#include <userver/formats/bson/document.hpp>
#include <userver/formats/bson/inline.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/yaml/serialize.hpp>
#include <userver/storages/mongo/bulk.hpp>
#include <userver/storages/mongo/collection.hpp>
#include <userver/storages/mongo/cursor.hpp>
#include <userver/storages/mongo/exception.hpp>
#include <userver/storages/mongo/operators.hpp>
#include <userver/storages/mongo/options.hpp>
#include <userver/storages/mongo/pool.hpp>
#include <userver/storages/mongo/pool_config.hpp>
#include <userver/storages/mongo/transaction.hpp>
#include <userver/testsuite/testpoint_control.hpp>
#include <userver/utils/async.hpp>
#include <userver/utils/fast_scope_guard.hpp>
#include <userver/yaml_config/yaml_config.hpp>

#include <dynamic_config/variables/MONGO_CONNECTION_POOL_SETTINGS.hpp>

USERVER_NAMESPACE_BEGIN

namespace mongo = storages::mongo;

namespace {
class Pool : public MongoPoolFixture {};
#ifdef USERVER_FEATURE_MONGO_EXPERIMENTAL
class ExperimentalPool : public Pool {};
#endif

bool WaitForClientCount(const mongo::impl::PoolImpl& pool, std::size_t expected) {
    const auto deadline = engine::Deadline::FromDuration(utest::kMaxTestWaitTime);
    while (pool.SizeApprox() != expected) {
        if (deadline.IsReached()) {
            return false;
        }
        engine::SleepFor(std::chrono::milliseconds{1});
    }
    return true;
}
}  // namespace

TEST(MongoPoolConfig, DriverImplDefaultsToAvailablePool) {
    const yaml_config::YamlConfig config{formats::yaml::FromString("{}"), {}};
#ifdef USERVER_FEATURE_MONGO_EXPERIMENTAL
    constexpr auto expected = mongo::PoolConfig::DriverImpl::kMongoCDriverExperimental;
#else
    constexpr auto expected = mongo::PoolConfig::DriverImpl::kMongoCDriver;
#endif
    EXPECT_EQ(config.As<mongo::PoolConfig>().driver_impl, expected);
    EXPECT_EQ(mongo::PoolConfig{}.driver_impl, expected);
}

TEST(MongoPoolConfig, DriverImplSelectsImplementation) {
    const yaml_config::YamlConfig legacy{formats::yaml::FromString("driver: mongo-c-driver"), {}};
    const yaml_config::YamlConfig experimental{formats::yaml::FromString("driver: mongo-c-driver-experimental"), {}};
    EXPECT_EQ(legacy.As<mongo::PoolConfig>().driver_impl, mongo::PoolConfig::DriverImpl::kMongoCDriver);
    EXPECT_EQ(
        experimental.As<mongo::PoolConfig>().driver_impl,
        mongo::PoolConfig::DriverImpl::kMongoCDriverExperimental
    );
}

TEST(MongoPoolConfig, DriverImplRejectsUnknownImplementation) {
    const yaml_config::YamlConfig config{formats::yaml::FromString("driver: unknown"), {}};
    UEXPECT_THROW(config.As<mongo::PoolConfig>(), mongo::InvalidConfigException);
}

TEST(MongoPoolConfig, ExperimentalDriverRequiresPatchedDriver) {
    mongo::PoolConfig config;
    config.driver_impl = mongo::PoolConfig::DriverImpl::kMongoCDriverExperimental;
#ifdef USERVER_FEATURE_MONGO_EXPERIMENTAL
    UEXPECT_NO_THROW(config.Validate("test"));
#else
    UEXPECT_THROW(config.Validate("test"), mongo::InvalidConfigException);
#endif
}

UTEST_P(Pool, CollectionAccess) {
    static const std::string kSysVerCollName = "system.version";
    static const std::string kNonexistentCollName = "nonexistent";

    // this database always exists
    auto admin_pool = MakePool("admin", {});
    // this one should not exist yet
    auto test_pool = MakePool(GetTestDatabaseDefaultName(), {});

    EXPECT_TRUE(admin_pool.HasCollection(kSysVerCollName));
    UEXPECT_NO_THROW(admin_pool.GetCollection(kSysVerCollName));

    EXPECT_FALSE(test_pool.HasCollection(kSysVerCollName));
    UEXPECT_NO_THROW(test_pool.GetCollection(kSysVerCollName));

    EXPECT_FALSE(admin_pool.HasCollection(kNonexistentCollName));
    UEXPECT_NO_THROW(admin_pool.GetCollection(kNonexistentCollName));

    EXPECT_FALSE(test_pool.HasCollection(kNonexistentCollName));
    UEXPECT_NO_THROW(test_pool.GetCollection(kNonexistentCollName));
}

UTEST_P(Pool, SharedClientAdapterOwnership) {
    const auto impl = GetPoolImpl(GetDefaultPool());
    const mongo::impl::cdriver::PoolAccess access{impl};
    const auto initial_in_use = impl->InUseApprox();
    {
        auto client = access.Acquire();
        EXPECT_EQ(impl->InUseApprox(), initial_in_use + 1);
        EXPECT_EQ(client.ShouldAccountErrorsWithoutEvents(), GetParam());
        auto* native = client.get();
        {
            auto borrowed = mongo::impl::cdriver::BoundClient::Borrowed(client);
            EXPECT_EQ(borrowed.get(), native);
            borrowed.reset();
            EXPECT_FALSE(borrowed);
            EXPECT_EQ(impl->InUseApprox(), initial_in_use + 1);
        }
        auto moved = std::move(client);
        // A moved-from client must no longer own the connection.
        // NOLINTNEXTLINE(bugprone-use-after-move)
        EXPECT_FALSE(client);
        EXPECT_EQ(moved.get(), native);
        EXPECT_EQ(impl->InUseApprox(), initial_in_use + 1);
        moved.reset();
        EXPECT_FALSE(moved);
        EXPECT_EQ(impl->InUseApprox(), initial_in_use);
    }
    EXPECT_EQ(impl->InUseApprox(), initial_in_use);
}

UTEST_P(Pool, DropDatabase) {
    static const std::string kCollName = "test";

    auto& pool = GetDefaultPool();
    auto coll = pool.GetCollection(kCollName);

    UEXPECT_NO_THROW(coll.InsertOne(formats::bson::MakeDoc("_id", 42)));
    EXPECT_TRUE(pool.HasCollection(kCollName));

    UEXPECT_NO_THROW(pool.DropDatabase());
    EXPECT_FALSE(pool.HasCollection(kCollName));

    UEXPECT_NO_THROW(coll.InsertOne(formats::bson::MakeDoc("_id", 42)));
    EXPECT_TRUE(pool.HasCollection(kCollName));
}

UTEST_P(Pool, ConnectionStringChangesDatabase) {
    using formats::bson::MakeDoc;
    const std::string k_other_database = GetTestDatabaseNamePrefix() + "uri_reload";
    const std::string k_collection = "uri_reload";
    auto old_pool = MakePool({}, {});
    auto new_pool = MakePool(k_other_database, {});
    auto pool = MakePool({}, {});
    auto collection = pool.GetCollection(k_collection);
    collection.InsertOne(MakeDoc("_id", 1));

    pool.SetConnectionString(GetTestsuiteMongoUri(k_other_database));
    EXPECT_FALSE(pool.HasCollection(k_collection));
    EXPECT_TRUE(pool.ListCollectionNames().empty());
    EXPECT_EQ(0, collection.Count({}));
    EXPECT_EQ(0, pool.GetCollection(k_collection).Count({}));

    collection.InsertOne(MakeDoc("_id", 2));
    collection.ReplaceOne(MakeDoc("_id", 2), MakeDoc("_id", 2, "value", 1));
    collection.UpdateOne(MakeDoc("_id", 2), MakeDoc("$set", MakeDoc("value", 2)));
    collection.ReplaceOne(
        MakeDoc("_id", 2),
        MakeDoc("_id", 2, "value", 1),
        mongo::options::MaxServerTime{utest::kMaxTestWaitTime}
    );
    collection.UpdateOne(
        MakeDoc("_id", 2),
        MakeDoc("$set", MakeDoc("value", 2)),
        mongo::options::MaxServerTime{utest::kMaxTestWaitTime}
    );
    mongo::operations::Bulk bulk(mongo::operations::Bulk::Mode::kOrdered);
    bulk.InsertOne(MakeDoc("_id", 3));
    collection.Execute(std::move(bulk));

    EXPECT_TRUE(pool.HasCollection(k_collection));
    EXPECT_EQ(std::vector<std::string>{k_collection}, pool.ListCollectionNames());
    EXPECT_EQ(1, old_pool.GetCollection(k_collection).Count({}));
    EXPECT_EQ(2, new_pool.GetCollection(k_collection).Count({}));
    EXPECT_EQ(1, new_pool.GetCollection(k_collection).Count(MakeDoc("value", 2)));

    pool.DropDatabase();
    EXPECT_FALSE(new_pool.HasCollection(k_collection));
    EXPECT_TRUE(old_pool.HasCollection(k_collection));

    pool.SetConnectionString(GetTestsuiteMongoUri(GetTestDatabaseDefaultName()));
    EXPECT_EQ(1, collection.Count({}));
}

UTEST_P(Pool, ConnectionStringChangesDatabaseWithMultipleConnections) {
    using formats::bson::MakeDoc;
    constexpr std::size_t k_initial_connections = 3;
    constexpr std::size_t k_busy_connections = 2;
    constexpr std::size_t k_max_connections = k_initial_connections + k_busy_connections;
    const std::string k_other_database = GetTestDatabaseNamePrefix() + "uri_reload_multiple_connections";
    const std::string k_collection = "uri_reload_multiple_connections";
    auto old_pool = MakePool({}, {});
    auto new_pool = MakePool(k_other_database, {});
    old_pool.GetCollection(k_collection).InsertMany({MakeDoc("database", "old"), MakeDoc("database", "old")});
    new_pool.GetCollection(k_collection).InsertMany({MakeDoc("database", "new"), MakeDoc("database", "new")});

    auto config = MakeTestPoolConfig();
    config.pool_settings.initial_size = k_initial_connections;
    config.pool_settings.max_size = k_max_connections;
    config.pool_settings.idle_limit = k_max_connections;
    auto pool = MakePool({}, config);
    auto collection = pool.GetCollection(k_collection);
    const auto pool_impl = GetPoolImpl(pool);
    std::vector<mongo::Cursor> old_cursors;
    old_cursors.reserve(k_busy_connections);
    for (std::size_t i = 0; i < k_busy_connections; ++i) {
        old_cursors.push_back(collection.Find({}, mongo::options::BatchSize{1}));
    }
    ASSERT_EQ(k_initial_connections, pool_impl->SizeApprox());
    ASSERT_EQ(k_busy_connections, pool_impl->InUseApprox());

    pool.SetConnectionString(GetTestsuiteMongoUri(k_other_database));
    std::vector<mongo::Cursor> new_cursors;
    new_cursors.reserve(k_initial_connections);
    for (std::size_t i = 0; i < k_initial_connections; ++i) {
        new_cursors.push_back(collection.Find({}, mongo::options::BatchSize{1}));
        ASSERT_EQ("new", (*new_cursors.back().begin())["database"].As<std::string>());
    }
    ASSERT_EQ(k_max_connections, pool_impl->InUseApprox());

    for (auto& cursor : old_cursors) {
        std::size_t count = 0;
        for (const auto& doc : cursor) {
            EXPECT_EQ("old", doc["database"].As<std::string>());
            ++count;
        }
        EXPECT_EQ(2, count);
    }
    old_cursors.clear();
    EXPECT_EQ(k_initial_connections, pool_impl->InUseApprox());
    ASSERT_TRUE(WaitForClientCount(*pool_impl, k_initial_connections));
    EXPECT_EQ(k_initial_connections, pool_impl->SizeApprox());

    for (auto& cursor : new_cursors) {
        std::size_t count = 0;
        for (const auto& doc : cursor) {
            EXPECT_EQ("new", doc["database"].As<std::string>());
            ++count;
        }
        EXPECT_EQ(2, count);
    }
    new_cursors.clear();
    EXPECT_EQ(0, pool_impl->InUseApprox());
    EXPECT_EQ(2, collection.Count(MakeDoc("database", "new")));
    EXPECT_EQ(0, collection.Count(MakeDoc("database", "old")));
}

UTEST_P(Pool, InvalidConnectionStringKeepsDatabase) {
    using formats::bson::MakeDoc;
    auto& pool = GetDefaultPool();
    auto collection = pool.GetCollection("uri_validation");
    collection.InsertOne(MakeDoc("_id", 1));

    for (const auto& uri : {std::string{"invalid-uri"}, GetTestsuiteMongoUri("")}) {
        for (int attempt = 0; attempt < 2; ++attempt) {
            UEXPECT_THROW(pool.SetConnectionString(uri), mongo::InvalidConfigException);
            EXPECT_EQ(1, collection.Count({}));
        }
    }
}

UTEST_P(Pool, ConnectionFailure) {
    auto dns_resolver = MakeDnsResolver();
    auto dynamic_config = MakeDynamicConfig();
    mongo::PoolConfig config;
    config.driver_impl =
        GetParam() ? mongo::PoolConfig::DriverImpl::kMongoCDriverExperimental
                   : mongo::PoolConfig::DriverImpl::kMongoCDriver;

    // constructor should not throw
    mongo::Pool bad_pool("bad", "mongodb://%2Fnonexistent.sock/bad", config, &dns_resolver, dynamic_config.GetSource());
    UEXPECT_THROW(bad_pool.HasCollection("test"), mongo::ClusterUnavailableException);
}

UTEST_P(Pool, DynamicPoolSettingsDoesNotAbort) {
    constexpr std::size_t kDynamicMaxSize = 32;
    const auto pool_settings = formats::json::FromString(R"({
        "userver_mongotest_dyn_named": {
            "max_size": 32,
            "idle_limit": 4,
            "initial_size": 1,
            "connecting_limit": 8
        },
        "__default__": {
            "max_size": 32,
            "idle_limit": 4,
            "initial_size": 1,
            "connecting_limit": 8
        }
    })");
    SetDynamicConfig({
        {::dynamic_config::MONGO_CONNECTION_POOL_SETTINGS, pool_settings},
    });

    auto named_pool = MakePool("userver_mongotest_dyn_named", {});
    EXPECT_EQ(kDynamicMaxSize, GetPoolImpl(named_pool)->MaxSize());

    auto fallback_pool = MakePool("userver_mongotest_dyn_fallback", {});
    EXPECT_EQ(kDynamicMaxSize, GetPoolImpl(fallback_pool)->MaxSize());
}

UTEST_P(Pool, Limits) {
    auto limited_config = MakeTestPoolConfig();
    limited_config.pool_settings.initial_size = 1;
    limited_config.pool_settings.idle_limit = 1;
    limited_config.pool_settings.max_size = 1;
    auto limited_pool = MakePool({}, limited_config);

    std::vector<formats::bson::Document> docs;
    docs.reserve(150);
    /// large enough to not fit into a single batch
    for (int i = 0; i < 150; ++i) {
        docs.push_back(formats::bson::MakeDoc("_id", i));
    }
    limited_pool.GetCollection("test").InsertMany(std::move(docs));

    auto cursor = limited_pool.GetCollection("test").Find({});

    auto second_find = engine::AsyncNoTracing([&limited_pool] { limited_pool.GetCollection("test").Find({}); });
    UEXPECT_THROW(second_find.Get(), mongo::MongoException);
    EXPECT_EQ(GetPoolImpl(limited_pool)->GetStatistics().pool->overload.Load().value, 1);
}

UTEST_P(Pool, ListCollectionNames) {
    static const std::string kCollAName = "list_test_a";
    static const std::string kCollBName = "list_test_b";

    auto& pool = GetDefaultPool();
    EXPECT_EQ(0, pool.ListCollectionNames().size());

    {
        auto coll = pool.GetCollection(kCollAName);
        UEXPECT_NO_THROW(coll.InsertOne(formats::bson::MakeDoc("_id", 42)));

        auto list_collections = pool.ListCollectionNames();
        EXPECT_EQ(1, list_collections.size());
        EXPECT_EQ(kCollAName, list_collections[0]);
    }
    {
        auto coll = pool.GetCollection(kCollBName);
        UEXPECT_NO_THROW(coll.InsertOne(formats::bson::MakeDoc("_id", 42)));

        auto list_collections = pool.ListCollectionNames();
        std::ranges::sort(list_collections);
        EXPECT_EQ(2, list_collections.size());
        EXPECT_EQ(kCollAName, list_collections[0]);
        EXPECT_EQ(kCollBName, list_collections[1]);
    }
    {
        auto coll = pool.GetCollection(kCollAName);
        UEXPECT_NO_THROW(coll.Drop());

        auto list_collections = pool.ListCollectionNames();
        EXPECT_EQ(1, list_collections.size());
        EXPECT_EQ(kCollBName, list_collections[0]);
    }
}

UTEST_P(Pool, AggregateDocuments) {
    using formats::bson::MakeArray;
    using formats::bson::MakeDoc;

    auto& pool = GetDefaultPool();

    /// [Sample Mongo database aggregate]
    auto cursor =
        pool.Aggregate(MakeArray(MakeDoc(mongo::operators::kDocuments, MakeArray(MakeDoc("x", 1), MakeDoc("x", 2)))));

    std::vector<int> values;
    for (const auto& doc : cursor) {
        values.push_back(doc["x"].As<int>());
    }
    /// [Sample Mongo database aggregate]

    ASSERT_EQ(2, values.size());
    EXPECT_EQ(1, values[0]);
    EXPECT_EQ(2, values[1]);
}

UTEST_P(Pool, AggregateDocumentsGroup) {
    using formats::bson::MakeArray;
    using formats::bson::MakeDoc;

    auto cursor = GetDefaultPool().Aggregate(MakeArray(
        MakeDoc(
            mongo::operators::kDocuments,
            MakeArray(MakeDoc("x", 1, "y", 10), MakeDoc("x", 2, "y", 20), MakeDoc("x", 1, "y", 30))
        ),
        MakeDoc(mongo::operators::kMatch, MakeDoc("x", 1)),
        MakeDoc(mongo::operators::kGroup, MakeDoc("_id", "$x", "sum", MakeDoc(mongo::operators::kSum, "$y")))
    ));

    auto it = cursor.begin();
    ASSERT_NE(it, cursor.end());
    const auto doc = *it;
    EXPECT_EQ(++it, cursor.end());
    EXPECT_EQ(1, doc["_id"].As<int>());
    EXPECT_EQ(40, doc["sum"].As<int>());
}

UTEST_P(Pool, AggregateDocumentsLookup) {
    using formats::bson::MakeArray;
    using formats::bson::MakeDoc;

    static const std::string kCollName = "aggregate_documents_lookup";
    auto& pool = GetDefaultPool();
    auto coll = pool.GetCollection(kCollName);
    coll.InsertMany({
        MakeDoc("qc_id", "1", "exam", "e1", "status", "ok"),
        MakeDoc("qc_id", "1", "exam", "e2", "status", "skip"),
        MakeDoc("qc_id", "2", "exam", "e1", "status", "ok"),
    });

    const auto pipeline = MakeArray(
        MakeDoc(mongo::operators::kDocuments, MakeArray(MakeDoc("qc_id", "1", "exam", "e1"))),
        MakeDoc(
            mongo::operators::kLookup,
            MakeDoc(
                "from",
                kCollName,
                "let",
                MakeDoc("qc_id", "$qc_id", "exam", "$exam"),
                "pipeline",
                MakeArray(MakeDoc(
                    mongo::operators::kMatch,
                    MakeDoc(
                        mongo::operators::kExpr,
                        MakeDoc(
                            mongo::operators::kAnd,
                            MakeArray(
                                MakeDoc(mongo::operators::kEq, MakeArray("$qc_id", "$$qc_id")),
                                MakeDoc(mongo::operators::kEq, MakeArray("$exam", "$$exam"))
                            )
                        )
                    )
                )),
                "as",
                "passes"
            )
        )
    );

    try {
        auto cursor = pool.Aggregate(pipeline);
        auto it = cursor.begin();
        ASSERT_NE(it, cursor.end());
        const auto doc = *it;
        EXPECT_EQ(++it, cursor.end());
        EXPECT_EQ("1", doc["qc_id"].As<std::string>());
        EXPECT_EQ("e1", doc["exam"].As<std::string>());
        ASSERT_EQ(1, doc["passes"].GetSize());
        EXPECT_EQ("ok", doc["passes"][0]["status"].As<std::string>());
    } catch (const mongo::ServerException& ex) {
        // Sharded MongoDB 6 cannot combine $documents (mongos-only) with $lookup (shard-only).
        const std::string_view message{ex.what()};
        if (message.find("$documents must run on mongoS") != std::string_view::npos) {
            GTEST_SKIP() << "Sharded MongoDB cannot combine $documents with $lookup: " << message;
        }
        throw;
    }
}

UTEST_P(Pool, AggregateDocumentsOnCollectionRejected) {
    using formats::bson::MakeArray;
    using formats::bson::MakeDoc;

    auto coll = GetDefaultPool().GetCollection("aggregate_documents_rejected");
    UEXPECT_THROW(
        coll.Aggregate(MakeArray(MakeDoc(mongo::operators::kDocuments, MakeArray(MakeDoc("x", 1))))),
        mongo::MongoException
    );
}

UTEST_P(Pool, AggregateInvalidPipeline) {
    auto& pool = GetDefaultPool();
    UEXPECT_THROW(pool.Aggregate(formats::bson::MakeArray()), mongo::InvalidQueryArgumentException);
    UEXPECT_THROW(pool.Aggregate(formats::bson::MakeDoc("x", 1)), mongo::InvalidQueryArgumentException);
}

#ifdef USERVER_FEATURE_MONGO_EXPERIMENTAL
UTEST_P(ExperimentalPool, QueueTimeoutReleasesWaiter) {
    auto config = MakeTestPoolConfig();
    config.pool_settings = {1, 1, 1, 1};
    config.queue_timeout = std::chrono::milliseconds{10};
    auto pool = MakePool({}, config);
    auto impl = std::static_pointer_cast<mongo::impl::cdriver_experimental::CDriverPoolImpl>(GetPoolImpl(pool));
    auto held = impl->Acquire();
    auto waiter = utils::Async("pool-waiter", [&] { UEXPECT_THROW(impl->Acquire(), mongo::PoolOverloadException); });
    waiter.WaitFor(std::chrono::seconds{1});
    EXPECT_TRUE(waiter.IsFinished());
    held.reset();
    waiter.Get();
    UEXPECT_NO_THROW(impl->Acquire());
}

UTEST_P(ExperimentalPool, CancelledWaiterReleasesPermit) {
    auto config = MakeTestPoolConfig();
    config.pool_settings = {1, 1, 1, 1};
    config.queue_timeout = std::chrono::seconds{30};
    auto pool = MakePool({}, config);
    auto impl = std::static_pointer_cast<mongo::impl::cdriver_experimental::CDriverPoolImpl>(GetPoolImpl(pool));
    auto held = impl->Acquire();
    engine::SingleConsumerEvent entered;
    auto waiter = utils::Async("cancelled-pool-waiter", [&] {
        entered.Send();
        UEXPECT_THROW(impl->Acquire(), mongo::CancelledException);
    });
    ASSERT_TRUE(entered.WaitForEvent());
    waiter.RequestCancel();
    waiter.WaitFor(std::chrono::seconds{1});
    EXPECT_TRUE(waiter.IsFinished());
    held.reset();
    waiter.Get();
    EXPECT_EQ(impl->InUseApprox(), 0);
    UEXPECT_NO_THROW(impl->Acquire());
}

UTEST_P(ExperimentalPool, DynamicLimitChangesNativePool) {
    auto config = MakeTestPoolConfig();
    config.pool_settings = {1, 1, 1, 1};
    config.maintenance_period = std::chrono::hours{1};
    auto pool = MakePool({}, config);
    auto impl = std::static_pointer_cast<mongo::impl::cdriver_experimental::CDriverPoolImpl>(GetPoolImpl(pool));
    auto first = impl->Acquire();
    impl->SetPoolSettings({1, 3, 3, 2});
    auto second = impl->Acquire();
    auto third = impl->Acquire();
    EXPECT_EQ(impl->SizeApprox(), 3);
    EXPECT_EQ(impl->InUseApprox(), 3);
    impl->SetMaxSize(1);
    UEXPECT_THROW(impl->Acquire(), mongo::PoolOverloadException);
    first.reset();
    ASSERT_TRUE(WaitForClientCount(*impl, 2));
    EXPECT_EQ(impl->SizeApprox(), 2);
    second.reset();
    EXPECT_EQ(impl->SizeApprox(), 1);
    UEXPECT_THROW(impl->Acquire(), mongo::PoolOverloadException);
    third.reset();
    UEXPECT_NO_THROW(impl->Acquire());
    EXPECT_EQ(impl->InUseApprox(), 0);
    EXPECT_EQ(impl->SizeApprox(), 1);
}

UTEST_P(ExperimentalPool, InitialSizeCreatesClientsWithoutPingAndShrinksIdle) {
    auto config = MakeTestPoolConfig();
    config.pool_settings = {3, 3, 3, 1};
    config.maintenance_period = std::chrono::hours{1};
    auto resolver = MakeDnsResolver();
    auto dynamic_config = MakeDynamicConfig();
    mongo::Pool pool(
        "initial-size",
        GetTestsuiteMongoUri(GetTestDatabaseDefaultName()) + "?minPoolSize=1",
        config,
        &resolver,
        dynamic_config.GetSource()
    );
    auto impl = GetPoolImpl(pool);
    EXPECT_EQ(impl->SizeApprox(), 3);
    EXPECT_EQ(impl->InUseApprox(), 0);
    EXPECT_EQ(impl->GetStatistics().pool->ping->GetCounter(mongo::stats::ErrorType::kSuccess).value, 0);
    impl->SetMaxSize(1);
    EXPECT_EQ(impl->SizeApprox(), 1);
    pool.Ping();
    EXPECT_EQ(impl->GetStatistics().pool->ping->GetCounter(mongo::stats::ErrorType::kSuccess).value, 1);
}

UTEST_P(ExperimentalPool, ZeroLimitCanRecoverAndShutdown) {
    auto config = MakeTestPoolConfig();
    config.pool_settings = {1, 1, 1, 1};
    auto pool = MakePool({}, config);
    auto impl = std::static_pointer_cast<mongo::impl::cdriver_experimental::CDriverPoolImpl>(GetPoolImpl(pool));
    pool.GetCollection("zero_limit").InsertOne(formats::bson::MakeDoc("x", 1));
    impl->SetMaxSize(0);
    EXPECT_EQ(impl->SizeApprox(), 0);
    UEXPECT_THROW(impl->Acquire(), mongo::PoolOverloadException);
    impl->SetMaxSize(1);
    UEXPECT_NO_THROW(impl->Acquire());
    impl->SetMaxSize(0);
}

UTEST_P(ExperimentalPool, NativeIdleLimitRemovesOnlyIdleClientsOnReturn) {
    auto config = MakeTestPoolConfig();
    config.pool_settings = {0, 3, 1, 1};
    config.maintenance_period = std::chrono::milliseconds{10};
    auto pool = MakePool({}, config);
    auto impl = std::static_pointer_cast<mongo::impl::cdriver_experimental::CDriverPoolImpl>(GetPoolImpl(pool));
    auto first = impl->Acquire();
    auto second = impl->Acquire();
    auto third = impl->Acquire();
    engine::SleepFor(std::chrono::milliseconds{50});
    EXPECT_EQ(impl->SizeApprox(), 3);
    first.reset();
    EXPECT_EQ(impl->SizeApprox(), 3);
    second.reset();
    EXPECT_EQ(impl->SizeApprox(), 2);
    EXPECT_EQ(impl->InUseApprox(), 1);
    third.reset();
    EXPECT_EQ(impl->SizeApprox(), 1);
    EXPECT_EQ(impl->GetStatistics().pool->ping->GetCounter(mongo::stats::ErrorType::kSuccess).value, 0);
    EXPECT_EQ(impl->GetStatistics().pool->created.Load().value, 3);
    EXPECT_EQ(impl->GetStatistics().pool->closed.Load().value, 2);
}

UTEST_P(ExperimentalPool, NativeIdleLimitChangesApplyOnReturnAndSurviveReload) {
    auto config = MakeTestPoolConfig();
    config.pool_settings = {3, 3, 3, 1};
    config.maintenance_period = std::chrono::milliseconds{10};
    auto pool = MakePool({}, config);
    auto impl = std::static_pointer_cast<mongo::impl::cdriver_experimental::CDriverPoolImpl>(GetPoolImpl(pool));
    impl->SetPoolSettings({0, 3, 1, 1});
    engine::SleepFor(std::chrono::milliseconds{50});
    EXPECT_EQ(impl->SizeApprox(), 3);
    impl->Acquire().reset();
    EXPECT_EQ(impl->SizeApprox(), 2);
    impl->Acquire().reset();
    EXPECT_EQ(impl->SizeApprox(), 1);
    pool.SetConnectionString(GetTestsuiteMongoUri(GetTestDatabaseDefaultName()) + "?retryReads=true");
    auto first = impl->Acquire();
    auto second = impl->Acquire();
    first.reset();
    second.reset();
    ASSERT_TRUE(WaitForClientCount(*impl, 1));
    EXPECT_EQ(impl->SizeApprox(), 1);
}

UTEST_P(ExperimentalPool, ZeroNativeIdleLimitDisablesIdlePruning) {
    auto config = MakeTestPoolConfig();
    config.pool_settings = {0, 2, 0, 1};
    config.maintenance_period = std::chrono::milliseconds{10};
    auto pool = MakePool({}, config);
    auto impl = std::static_pointer_cast<mongo::impl::cdriver_experimental::CDriverPoolImpl>(GetPoolImpl(pool));
    auto first = impl->Acquire();
    auto second = impl->Acquire();
    first.reset();
    second.reset();
    engine::SleepFor(std::chrono::milliseconds{50});
    EXPECT_EQ(impl->SizeApprox(), 2);
    impl->SetPoolSettings({0, 2, 1, 1});
    impl->Acquire().reset();
    EXPECT_EQ(impl->SizeApprox(), 1);
    impl->SetPoolSettings({0, 2, 0, 1});
    auto third = impl->Acquire();
    auto fourth = impl->Acquire();
    third.reset();
    fourth.reset();
    EXPECT_EQ(impl->SizeApprox(), 2);
}

UTEST_P(ExperimentalPool, ConnectingLimitDefaultsNativeUriAndUpdatesGeneration) {
    auto config = MakeTestPoolConfig();
    config.pool_settings = {0, 4, 0, 3};
    auto pool = MakePool({}, config);
    auto impl = std::static_pointer_cast<mongo::impl::cdriver_experimental::CDriverPoolImpl>(GetPoolImpl(pool));
    auto original = impl->Acquire();
    EXPECT_EQ(original.GetPool()->GetMaxConnecting(), 3);
    EXPECT_EQ(mongoc_uri_get_option_as_int32(mongoc_client_get_uri(original.get()), MONGOC_URI_MAXCONNECTING, 0), 3);
    impl->SetPoolSettings({0, 4, 0, 1});
    auto current = impl->Acquire();
    EXPECT_NE(current.GetPool(), original.GetPool());
    EXPECT_EQ(current.GetPool()->GetMaxConnecting(), 1);
    EXPECT_EQ(original.GetPool()->GetMaxConnecting(), 3);
    impl->SetPoolSettings({0, 4, 0, 1});
    EXPECT_EQ(impl->Acquire().GetPool(), current.GetPool());
    original.reset();
    current.reset();
    pool.Ping();
}

UTEST_P(ExperimentalPool, ConnectingLimitPreservesExplicitUriAndLatestDefaultOnReload) {
    auto config = MakeTestPoolConfig();
    config.pool_settings = {0, 4, 0, 3};
    auto pool = MakePool({}, config);
    auto impl = std::static_pointer_cast<mongo::impl::cdriver_experimental::CDriverPoolImpl>(GetPoolImpl(pool));
    const auto uri = GetTestsuiteMongoUri(GetTestDatabaseDefaultName());
    pool.SetConnectionString(uri + "?mAxCoNnEcTiNg=5");
    auto explicit_client = impl->Acquire();
    EXPECT_EQ(explicit_client.GetPool()->GetMaxConnecting(), 5);
    impl->SetPoolSettings({0, 4, 0, 1});
    EXPECT_EQ(impl->Acquire().GetPool(), explicit_client.GetPool());
    EXPECT_EQ(explicit_client.GetPool()->GetMaxConnecting(), 5);
    pool.SetConnectionString(uri);
    auto default_client = impl->Acquire();
    EXPECT_EQ(default_client.GetPool()->GetMaxConnecting(), 1);
    EXPECT_NE(default_client.GetPool(), explicit_client.GetPool());
}

UTEST_P(ExperimentalPool, ConnectingLimitRejectsUnrepresentableDefaultWithoutRetiringPool) {
    auto config = MakeTestPoolConfig();
    config.pool_settings = {0, 4, 0, 1};
    auto pool = MakePool({}, config);
    auto impl = std::static_pointer_cast<mongo::impl::cdriver_experimental::CDriverPoolImpl>(GetPoolImpl(pool));
    auto original = impl->Acquire();
    auto settings = config.pool_settings;
    settings.connecting_limit = static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()) + 1;
    UEXPECT_THROW(impl->SetPoolSettings(settings), mongo::InvalidConfigException);
    EXPECT_EQ(impl->Acquire().GetPool(), original.GetPool());
    EXPECT_EQ(original.GetPool()->GetMaxConnecting(), 1);
    impl->SetPoolSettings(config.pool_settings);
    pool.Ping();
}

UTEST_P(ExperimentalPool, LazyClientsIgnoreConnectingLimit) {
    auto config = MakeTestPoolConfig();
    config.pool_settings = {0, 3, 0, 1};
    config.queue_timeout = std::chrono::seconds{1};
    auto pool = MakePool({}, config);
    auto impl = std::static_pointer_cast<mongo::impl::cdriver_experimental::CDriverPoolImpl>(GetPoolImpl(pool));
    testsuite::TestpointControl control;
    class BlockFirstConnection final : public testsuite::TestpointClientBase {
    public:
        ~BlockFirstConnection() override { Unregister(); }
        void Execute(std::string_view, const formats::json::Value&, Callback) const override {
            if (++calls == 1) {
                entered.Send();
                EXPECT_TRUE(release.WaitForEvent());
            }
        }
        mutable std::atomic<int> calls{0};
        mutable engine::SingleConsumerEvent entered;
        mutable engine::SingleConsumerEvent release;
    } blocker;
    control.SetClient(blocker);
    control.SetEnabledNames({"mongo-client-acquired"});
    auto first_task = utils::Async("first-pool-connection", [&] { return impl->Acquire(); });
    const utils::FastScopeGuard release_blocker([&]() noexcept { blocker.release.Send(); });
    ASSERT_TRUE(blocker.entered.WaitForEvent());
    auto second = impl->Acquire();
    auto third = impl->Acquire();
    EXPECT_EQ(impl->InUseApprox(), 3);
    UEXPECT_THROW(impl->Acquire(), mongo::PoolOverloadException);
    engine::SingleConsumerEvent waiter_entered;
    auto cancelled_waiter = utils::Async("cancelled-pool-waiter", [&] {
        waiter_entered.Send();
        UEXPECT_THROW(impl->Acquire(), mongo::CancelledException);
    });
    ASSERT_TRUE(waiter_entered.WaitForEvent());
    cancelled_waiter.RequestCancel();
    cancelled_waiter.Get();
    EXPECT_EQ(impl->InUseApprox(), 3);
    impl->SetPoolSettings({0, 4, 0, 1});
    auto fourth = impl->Acquire();
    EXPECT_EQ(blocker.calls.load(), 4);
    EXPECT_EQ(impl->InUseApprox(), 4);
    blocker.release.Send();
    auto first = first_task.Get();
}

TEST(MongoPoolConfig, RejectsUnrepresentableNativeLimit) {
    if (std::numeric_limits<std::size_t>::max() > std::numeric_limits<std::uint32_t>::max()) {
        mongo::PoolSettings settings;
        settings.max_size = std::numeric_limits<std::size_t>::max();
        UEXPECT_THROW(settings.Validate("test"), mongo::InvalidConfigException);
    }
}

UTEST_P(ExperimentalPool, LegacyAndExperimentalPoolsCoexist) {
    auto resolver = MakeDnsResolver();
    auto dynamic_config = MakeDynamicConfig();
    mongo::PoolConfig config;
    config.driver_impl = mongo::PoolConfig::DriverImpl::kMongoCDriver;
    config.pool_settings.initial_size = 1;
    mongo::Pool legacy(
        "legacy",
        GetTestsuiteMongoUri(GetTestDatabaseDefaultName()),
        config,
        &resolver,
        dynamic_config.GetSource()
    );

    EXPECT_TRUE(std::dynamic_pointer_cast<mongo::impl::cdriver::CDriverPoolImpl>(GetPoolImpl(legacy)));
    EXPECT_TRUE(std::dynamic_pointer_cast<
                mongo::impl::cdriver_experimental::CDriverPoolImpl>(GetPoolImpl(GetDefaultPool())));

    auto legacy_collection = legacy.GetCollection("dual_pool");
    auto experimental_collection = GetDefaultPool().GetCollection("dual_pool");
    legacy_collection.InsertOne(formats::bson::MakeDoc("_id", 1));
    EXPECT_EQ(experimental_collection.Count({}), 1);
    experimental_collection.InsertOne(formats::bson::MakeDoc("_id", 2));
    EXPECT_EQ(legacy_collection.Count({}), 2);

    const auto new_database = GetTestDatabaseNamePrefix() + "dual_reload";
    auto new_database_pool = MakePool(new_database, {});
    legacy.SetConnectionString(GetTestsuiteMongoUri(new_database));
    EXPECT_EQ(legacy_collection.Count({}), 0);
    legacy_collection.InsertOne(formats::bson::MakeDoc("_id", 3));
    EXPECT_EQ(new_database_pool.GetCollection("dual_pool").Count({}), 1);
    legacy.SetConnectionString(GetTestsuiteMongoUri(GetTestDatabaseDefaultName()));
    EXPECT_EQ(legacy_collection.Count({}), 2);
}

UTEST(NonexistentPool, ClientsConnectOnFirstOperation) {
    auto resolver = MakeDnsResolver();
    auto dynamic_config = MakeDynamicConfig();
    auto config = MakePoolConfigForTest(true);
    config.pool_settings = {1, 2, 2, 1};
    mongo::Pool pool(
        "bad-warmup",
        "mongodb://%2Fnonexistent.sock/bad?serverSelectionTimeoutMS=10",
        config,
        &resolver,
        dynamic_config.GetSource()
    );
    auto impl = std::static_pointer_cast<mongo::impl::cdriver_experimental::CDriverPoolImpl>(GetPoolImpl(pool));
    EXPECT_EQ(impl->SizeApprox(), 1);
    {
        auto initial = impl->Acquire();
        auto created = impl->Acquire();
        EXPECT_EQ(impl->InUseApprox(), 2);
        EXPECT_EQ(impl->SizeApprox(), 2);
    }
    EXPECT_EQ(impl->InUseApprox(), 0);
    EXPECT_EQ(impl->GetStatistics().pool->created.Load().value, 2);
    EXPECT_EQ(impl->GetStatistics().pool->closed.Load().value, 0);
    UEXPECT_THROW(pool.Ping(), mongo::ClusterUnavailableException);
    EXPECT_EQ(impl->InUseApprox(), 0);
}

UTEST_P(ExperimentalPool, ReloadChangesNativeUri) {
    auto pool = MakePool({}, {});
    auto impl = std::static_pointer_cast<mongo::impl::cdriver_experimental::CDriverPoolImpl>(GetPoolImpl(pool));
    auto old_client = impl->Acquire();
    EXPECT_FALSE(mongoc_uri_get_option_as_bool(mongoc_client_get_uri(old_client.get()), MONGOC_URI_RETRYREADS, false));
    pool.SetConnectionString(GetTestsuiteMongoUri(GetTestDatabaseDefaultName()) + "?retryReads=true");
    auto new_client = impl->Acquire();
    EXPECT_TRUE(mongoc_uri_get_option_as_bool(mongoc_client_get_uri(new_client.get()), MONGOC_URI_RETRYREADS, false));
    EXPECT_FALSE(mongoc_uri_get_option_as_bool(mongoc_client_get_uri(old_client.get()), MONGOC_URI_RETRYREADS, false));
}

UTEST_P(ExperimentalPool, ReloadRetainsOnlyBorrowedGeneration) {
    auto config = MakeTestPoolConfig();
    config.pool_settings = {2, 4, 4, 2};
    auto pool = MakePool({}, config);
    auto impl = std::static_pointer_cast<mongo::impl::cdriver_experimental::CDriverPoolImpl>(GetPoolImpl(pool));
    auto old_client = impl->Acquire();
    std::weak_ptr old_generation = old_client.GetPool();
    pool.SetConnectionString(GetTestsuiteMongoUri(GetTestDatabaseDefaultName()) + "?retryReads=true");
    EXPECT_FALSE(old_generation.expired());
    EXPECT_EQ(impl->SizeApprox(), 1);
    auto new_client = impl->Acquire();
    EXPECT_NE(new_client.GetPool(), old_client.GetPool());
    impl->MarkBulkWriteUnsupported(old_client);
    EXPECT_EQ(
        new_client.GetPool()->GetBulkWriteSupport(),
        mongo::impl::cdriver_experimental::CDriverPoolImpl::BulkWriteSupport::kUnknown
    );
    EXPECT_EQ(impl->SizeApprox(), 2);
    old_client.reset();
    EXPECT_TRUE(old_generation.expired());
    ASSERT_TRUE(WaitForClientCount(*impl, 1));
    EXPECT_EQ(impl->SizeApprox(), 1);
    EXPECT_EQ(impl->InUseApprox(), 1);
}

UTEST_P(ExperimentalPool, RetiredGenerationCleanupDoesNotBlockClientReturn) {
    auto config = MakeTestPoolConfig();
    config.pool_settings = {0, 1, 0, 1};
    std::optional<mongo::Pool> pool{MakePool({}, config)};
    auto impl = std::static_pointer_cast<mongo::impl::cdriver_experimental::CDriverPoolImpl>(GetPoolImpl(*pool));
    pool->Ping();
    auto old_client = impl->Acquire();
    pool->SetConnectionString(GetTestsuiteMongoUri(GetTestDatabaseDefaultName()) + "?retryReads=true");

    testsuite::TestpointControl control;
    class BlockFirstDestruction final : public testsuite::TestpointClientBase {
    public:
        ~BlockFirstDestruction() override { Unregister(); }
        void Execute(std::string_view, const formats::json::Value&, Callback) const override {
            if (++calls == 1) {
                entered.Send();
                EXPECT_TRUE(release.WaitForEvent());
            }
        }
        mutable std::atomic<int> calls{0};
        mutable engine::SingleConsumerEvent entered;
        mutable engine::SingleConsumerEvent release;
    } blocker;
    control.SetClient(blocker);
    control.SetEnabledNames({"mongo-pool-generation-destroying"});
    engine::SingleConsumerEvent returned;
    auto returner = utils::Async("return-old-mongo-client", [&returned, client = std::move(old_client)]() mutable {
        client.reset();
        returned.Send();
    });
    const utils::FastScopeGuard release_blocker([&]() noexcept { blocker.release.Send(); });
    ASSERT_TRUE(blocker.entered.WaitForEventFor(utest::kMaxTestWaitTime));
    ASSERT_TRUE(returned.WaitForEventFor(utest::kMaxTestWaitTime));
    returner.Get();
    EXPECT_EQ(impl->InUseApprox(), 0);
    impl->Acquire().reset();

    engine::SingleConsumerEvent shutdown_started;
    engine::SingleConsumerEvent shutdown_finished;
    auto shutdown = utils::Async("destroy-mongo-pool", [&] {
        shutdown_started.Send();
        pool.reset();
        impl.reset();
        shutdown_finished.Send();
    });
    const utils::FastScopeGuard release_before_join([&]() noexcept { blocker.release.Send(); });
    ASSERT_TRUE(shutdown_started.WaitForEventFor(utest::kMaxTestWaitTime));
    EXPECT_FALSE(shutdown_finished.WaitForEventFor(std::chrono::milliseconds{20}));
    blocker.release.Send();
    shutdown.Get();
    EXPECT_TRUE(shutdown_finished.IsReady());
}

UTEST_P(ExperimentalPool, ReloadInvalidUriPreservesWorkingGeneration) {
    auto pool = MakePool({}, {});
    auto impl = std::static_pointer_cast<mongo::impl::cdriver_experimental::CDriverPoolImpl>(GetPoolImpl(pool));
    auto original = impl->Acquire();
    for (int i = 0; i < 2; ++i) {
        UEXPECT_THROW(pool.SetConnectionString("not-a-mongodb-uri"), mongo::InvalidConfigException);
        auto current = impl->Acquire();
        EXPECT_EQ(current.GetPool(), original.GetPool());
        EXPECT_EQ(impl->DefaultDatabaseName(), GetTestDatabaseDefaultName());
    }
    UEXPECT_THROW(pool.SetConnectionString("mongodb://localhost"), mongo::InvalidConfigException);
    pool.SetConnectionString(GetTestsuiteMongoUri(GetTestDatabaseDefaultName()));
    auto unchanged = impl->Acquire();
    EXPECT_EQ(unchanged.GetPool(), original.GetPool());
    pool.Ping();
}

#endif
UTEST_P(Pool, ReloadChangesDatabaseWithActiveCursorAndTransaction) {
    const auto new_database = GetTestDatabaseNamePrefix() + "reload_new";
    auto new_database_pool = MakePool(new_database, {});
    auto pool = MakePool({}, {});
    auto old_collection = pool.GetCollection("generation_cursor");
    old_collection.InsertMany(
        {formats::bson::MakeDoc("_id", 1), formats::bson::MakeDoc("_id", 2), formats::bson::MakeDoc("_id", 3)}
    );
    auto cursor = old_collection.Find({}, mongo::options::BatchSize{1});
    auto transaction = pool.BeginTransaction();
    auto transaction_collection = transaction.GetCollection("generation_transaction");
    transaction_collection.InsertOne(formats::bson::MakeDoc("_id", 1));

    pool.SetConnectionString(GetTestsuiteMongoUri(new_database));
    EXPECT_EQ(GetPoolImpl(pool)->DefaultDatabaseName(), new_database);
    EXPECT_FALSE(pool.HasCollection("generation_cursor"));
    pool.GetCollection("generation_cursor").InsertOne(formats::bson::MakeDoc("_id", 42));
    EXPECT_EQ(new_database_pool.GetCollection("generation_cursor").Count({}), 1);
    EXPECT_EQ(old_collection.Count({}), 1);
    EXPECT_EQ(old_collection.Count(formats::bson::MakeDoc("_id", 42)), 1);
    EXPECT_EQ(GetDefaultPool().GetCollection("generation_cursor").Count({}), 3);
    int old_documents = 0;
    for (const auto& doc : cursor) {
        EXPECT_NE(doc["_id"].As<int>(), 42);
        ++old_documents;
    }
    EXPECT_EQ(old_documents, 3);
    transaction_collection.InsertOne(formats::bson::MakeDoc("_id", 2));
    transaction.Commit();
    EXPECT_EQ(GetDefaultPool().GetCollection("generation_transaction").Count({}), 2);
    EXPECT_EQ(pool.GetCollection("generation_transaction").Count({}), 0);
}

#ifdef USERVER_FEATURE_MONGO_EXPERIMENTAL
UTEST_P(ExperimentalPool, CursorSurvivesPoolDestruction) {
    std::optional<mongo::Cursor> cursor;
    std::weak_ptr<mongo::impl::PoolImpl> owner;
    {
        auto pool = MakePool({}, {});
        owner = GetPoolImpl(pool);
        auto collection = pool.GetCollection("orphan_cursor");
        collection.InsertMany({formats::bson::MakeDoc("_id", 1), formats::bson::MakeDoc("_id", 2)});
        cursor.emplace(collection.Find({}, mongo::options::BatchSize{1}));
    }
    EXPECT_TRUE(owner.expired());
    int documents = 0;
    for ([[maybe_unused]] const auto& doc : *cursor) {
        ++documents;
    }
    EXPECT_EQ(documents, 2);
    cursor.reset();
}
#endif

#ifdef USERVER_FEATURE_MONGO_EXPERIMENTAL
UTEST_P(ExperimentalPool, ReloadOwnsSslAndApmContext) {
    auto pool = MakePool({}, {});
    auto impl = std::static_pointer_cast<mongo::impl::cdriver_experimental::CDriverPoolImpl>(GetPoolImpl(pool));
    const auto uri = GetTestsuiteMongoUri(GetTestDatabaseDefaultName());
    pool.SetConnectionString(uri + "?tls=false&tlsCAFile=old-ca.pem");
    auto old_client = impl->Acquire();
    auto* old_context = &old_client.GetPool()->GetInitiatorData();
    auto* old_stats = old_client.GetPool()->GetStatsPtr();
    pool.SetConnectionString(uri + "?tls=false&tlsCAFile=new-ca.pem");
    auto new_client = impl->Acquire();
    EXPECT_NE(old_context, &new_client.GetPool()->GetInitiatorData());
    EXPECT_STREQ(old_context->ssl_opt.ca_file, "old-ca.pem");
    EXPECT_STREQ(new_client.GetPool()->GetInitiatorData().ssl_opt.ca_file, "new-ca.pem");
    EXPECT_NE(old_stats, new_client.GetPool()->GetStatsPtr());
    EXPECT_EQ(old_stats->apm_stats, new_client.GetPool()->GetStatsPtr()->apm_stats);
    EXPECT_EQ(old_stats->apm_stats, &impl->GetApmStats());
}

UTEST_P(ExperimentalPool, ReloadWhileClientIsBeingAcquired) {
    auto config = MakeTestPoolConfig();
    config.pool_settings = {0, 2, 0, 1};
    config.queue_timeout = utest::kMaxTestWaitTime;
    auto pool = MakePool({}, config);
    auto impl = std::static_pointer_cast<mongo::impl::cdriver_experimental::CDriverPoolImpl>(GetPoolImpl(pool));
    testsuite::TestpointControl control;
    class BlockFirstConnection final : public testsuite::TestpointClientBase {
    public:
        ~BlockFirstConnection() override { Unregister(); }
        void Execute(std::string_view, const formats::json::Value&, Callback) const override {
            if (++calls == 1) {
                entered.Send();
                EXPECT_TRUE(release.WaitForEvent());
            }
        }
        mutable std::atomic<int> calls{0};
        mutable engine::SingleConsumerEvent entered;
        mutable engine::SingleConsumerEvent release;
    } blocker;
    control.SetClient(blocker);
    control.SetEnabledNames({"mongo-client-acquired"});
    auto first_task = utils::Async("old-generation-create", [&] { return impl->Acquire(); });
    const utils::FastScopeGuard release_blocker([&]() noexcept { blocker.release.Send(); });
    ASSERT_TRUE(blocker.entered.WaitForEvent());
    pool.SetConnectionString(GetTestsuiteMongoUri(GetTestDatabaseDefaultName()) + "?retryReads=true");
    auto second = impl->Acquire();
    impl->SetMaxSize(3);
    blocker.release.Send();
    auto first = first_task.Get();
    EXPECT_NE(first.GetPool(), second.GetPool());
    EXPECT_TRUE(mongoc_uri_get_option_as_bool(mongoc_client_get_uri(second.get()), MONGOC_URI_RETRYREADS, false));
    EXPECT_FALSE(mongoc_uri_get_option_as_bool(mongoc_client_get_uri(first.get()), MONGOC_URI_RETRYREADS, false));
    auto third = impl->Acquire();
    EXPECT_EQ(second.GetPool(), third.GetPool());
}

UTEST_P(ExperimentalPool, EndSessionsOnShutdownAndReload) {
    testsuite::TestpointControl control;
    class EndSessionsObserver final : public testsuite::TestpointClientBase {
    public:
        ~EndSessionsObserver() override { Unregister(); }
        void Execute(std::string_view, const formats::json::Value&, Callback) const override { ++calls; }
        mutable std::atomic<int> calls{0};
    } observer;
    control.SetClient(observer);
    control.SetEnabledNames({"mongo-pool-end-sessions-succeeded"});
    for (bool reload : {false, true}) {
        SCOPED_TRACE(reload);
        const auto before = observer.calls.load();
        std::shared_ptr<mongo::stats::PoolConnectStatistics> stats;
        {
            auto config = MakeTestPoolConfig();
            config.pool_settings = {0, 2, 0, 1};
            auto pool = MakePool({}, config);
            auto impl = std::static_pointer_cast<mongo::impl::cdriver_experimental::CDriverPoolImpl>(GetPoolImpl(pool));
            stats = impl->GetStatistics().pool;
            pool.GetCollection("end_sessions").InsertOne(formats::bson::MakeDoc("value", 1));
            auto client = impl->Acquire();
            if (reload) {
                pool.SetConnectionString(GetTestsuiteMongoUri(GetTestDatabaseDefaultName()) + "?retryReads=true");
                EXPECT_EQ(observer.calls.load(), before);
            }
            client.reset();
        }
        EXPECT_EQ(observer.calls.load(), before + 1);
        EXPECT_EQ(stats->current_size.load(), 0);
        EXPECT_EQ(stats->created.Load(), stats->closed.Load());
    }
}

UTEST_P(ExperimentalPool, MetricsFinalSnapshotOnDestruction) {
    auto config = MakeTestPoolConfig();
    config.pool_settings = {0, 2, 0, 1};
    std::shared_ptr<mongo::stats::PoolConnectStatistics> stats;
    {
        auto pool = MakePool({}, config);
        auto impl = std::static_pointer_cast<mongo::impl::cdriver_experimental::CDriverPoolImpl>(GetPoolImpl(pool));
        stats = impl->GetStatistics().pool;
        auto client = impl->Acquire();
        client.reset();
        EXPECT_EQ(stats->created.Load().value, 1);
        EXPECT_EQ(stats->closed.Load().value, 0);
        EXPECT_EQ(stats->current_size.load(), 1);
    }
    EXPECT_EQ(stats->created.Load().value, 1);
    EXPECT_EQ(stats->closed.Load().value, 1);
    EXPECT_EQ(stats->current_size.load(), 0);
}

UTEST_P(ExperimentalPool, MetricsSnapshotsAcrossMultipleGenerations) {
    auto config = MakeTestPoolConfig();
    config.pool_settings = {0, 3, 0, 1};
    auto pool = MakePool({}, config);
    auto impl = std::static_pointer_cast<mongo::impl::cdriver_experimental::CDriverPoolImpl>(GetPoolImpl(pool));
    const auto stats = impl->GetStatistics().pool;
    auto first = impl->Acquire();
    first.GetPool()->RefreshStatistics();
    first.GetPool()->RefreshStatistics();
    EXPECT_EQ(stats->created.Load().value, 1);
    pool.SetConnectionString(GetTestsuiteMongoUri(GetTestDatabaseDefaultName()) + "?retryReads=true");
    auto second = impl->Acquire();
    pool.SetConnectionString(GetTestsuiteMongoUri(GetTestDatabaseDefaultName()) + "?retryWrites=true");
    auto third = impl->Acquire();
    EXPECT_EQ(stats->created.Load().value, 3);
    EXPECT_EQ(stats->closed.Load().value, 0);
    EXPECT_EQ(impl->SizeApprox(), 3);
    second.reset();
    ASSERT_TRUE(WaitForClientCount(*impl, 2));
    EXPECT_EQ(stats->closed.Load().value, 1);
    EXPECT_EQ(impl->SizeApprox(), 2);
    first.reset();
    ASSERT_TRUE(WaitForClientCount(*impl, 1));
    EXPECT_EQ(stats->closed.Load().value, 2);
    EXPECT_EQ(impl->SizeApprox(), 1);
    third.reset();
    impl->SetMaxSize(0);
    EXPECT_EQ(stats->created.Load().value, 3);
    EXPECT_EQ(stats->closed.Load().value, 3);
    EXPECT_EQ(impl->SizeApprox(), 0);
}

UTEST_P(ExperimentalPool, MetricsClientLifecycleAcrossReloadAndShutdown) {
    auto config = MakeTestPoolConfig();
    config.pool_settings = {2, 3, 3, 1};
    config.maintenance_period = std::chrono::hours{1};
    std::shared_ptr<mongo::stats::PoolConnectStatistics> stats;
    {
        auto pool = MakePool({}, config);
        auto impl = std::static_pointer_cast<mongo::impl::cdriver_experimental::CDriverPoolImpl>(GetPoolImpl(pool));
        stats = impl->GetStatistics().pool;
        EXPECT_EQ(stats->created.Load().value, 2);
        EXPECT_EQ(stats->closed.Load().value, 0);
        auto first = impl->Acquire();
        auto second = impl->Acquire();
        auto third = impl->Acquire();
        EXPECT_EQ(stats->created.Load().value, 3);
        EXPECT_EQ(stats->requested.Load().value, 3);
        EXPECT_EQ(impl->InUseApprox(), 3);
        impl->SetMaxSize(2);
        third.reset();
        EXPECT_EQ(stats->closed.Load().value, 1);
        second.reset();
        pool.SetConnectionString(GetTestsuiteMongoUri(GetTestDatabaseDefaultName()) + "?retryReads=true");
        EXPECT_EQ(impl->SizeApprox(), 1);
        EXPECT_EQ(stats->closed.Load().value, 2);
        auto replacement = impl->Acquire();
        EXPECT_EQ(impl->SizeApprox(), 2);
        EXPECT_EQ(impl->InUseApprox(), 2);
        first.reset();
        ASSERT_TRUE(WaitForClientCount(*impl, 1));
        EXPECT_EQ(impl->SizeApprox(), 1);
        EXPECT_EQ(stats->created.Load().value - stats->closed.Load().value, impl->SizeApprox());
        replacement.reset();
        pool.GetCollection("shutdown_metrics").InsertOne(formats::bson::MakeDoc("_id", 1));
        EXPECT_GT(impl->GetApmStats().heartbeats.success.Load().value, 0);
        EXPECT_GT(impl->GetApmStats().topology.changed.Load().value, 0);
        impl->SetMaxSize(0);
        EXPECT_EQ(stats->current_size.load(), 0);
        EXPECT_EQ(stats->created.Load(), stats->closed.Load());
    }
    EXPECT_EQ(stats->current_size.load(), 0);
    EXPECT_EQ(stats->created.Load(), stats->closed.Load());
}

#endif
UTEST_P_MT(Pool, MetricsCursorCountsBatchesAndIsolatesConcurrentCommands, 4) {
    auto pool = MakePool({}, {});
    auto impl = GetPoolImpl(pool);
    auto collection = pool.GetCollection("cursor_metrics");
    for (int i = 0; i < 5; ++i) {
        collection.InsertOne(formats::bson::MakeDoc("_id", i));
    }
    const auto stats = impl->GetStatistics().collections["cursor_metrics"]->items[mongo::stats::OperationKey{
        mongo::stats::OpType::kFind
    }];
    auto cursor = collection.Find({}, mongo::options::BatchSize{2});
    EXPECT_EQ(stats->GetTotalQueries().value, 1);
    auto iter = cursor.begin();
    const auto before = mongo::stats::GetTaskEventStats();
    auto other = utils::Async("unrelated-command", [&] { pool.Ping(); });
    other.Get();
    EXPECT_EQ(mongo::stats::GetTaskEventStats(), before);
    ++iter;
    EXPECT_EQ(stats->GetTotalQueries().value, 1);
    ++iter;
    EXPECT_EQ(stats->GetTotalQueries().value, 2);
    ++iter;
    EXPECT_EQ(stats->GetTotalQueries().value, 2);
    ++iter;
    EXPECT_EQ(stats->GetTotalQueries().value, 3);
    ++iter;
    EXPECT_EQ(iter, cursor.end());
    EXPECT_EQ(stats->GetTotalQueries().value, 3);
}

#ifdef USERVER_FEATURE_MONGO_EXPERIMENTAL
UTEST_P_MT(ExperimentalPool, MetricsConcurrentClientLifecycle, 4) {
    auto config = MakeTestPoolConfig();
    constexpr std::size_t kWorkers = 4;
    constexpr std::size_t kIterations = 8;
    config.pool_settings = {0, kWorkers, 0, kWorkers};
    config.queue_timeout = std::chrono::seconds{5};
    config.maintenance_period = std::chrono::hours{1};
    auto pool = MakePool({}, config);
    auto impl = std::static_pointer_cast<mongo::impl::cdriver_experimental::CDriverPoolImpl>(GetPoolImpl(pool));
    std::vector<engine::TaskWithResult<void>> tasks;
    tasks.reserve(kWorkers);
    for (std::size_t i = 0; i < kWorkers; ++i) {
        tasks.push_back(utils::Async("client-lifecycle", [&] {
            for (std::size_t j = 0; j < kIterations; ++j) {
                auto client = impl->Acquire();
                engine::Yield();
                client.reset();
            }
        }));
    }
    for (auto& task : tasks) {
        task.Get();
    }
    const auto& stats = *impl->GetStatistics().pool;
    EXPECT_EQ(stats.requested.Load().value, kWorkers * kIterations);
    EXPECT_EQ(impl->InUseApprox(), 0);
    EXPECT_EQ(stats.created.Load().value - stats.closed.Load().value, impl->SizeApprox());
    impl->SetMaxSize(0);
    EXPECT_EQ(impl->SizeApprox(), 0);
    EXPECT_EQ(stats.created.Load(), stats.closed.Load());
}
#endif

INSTANTIATE_UTEST_SUITE_P(
    Driver,
    Pool,
    ::testing::ValuesIn(GetMongoPoolImplementations()),
    GetMongoPoolImplementationName
);
#ifdef USERVER_FEATURE_MONGO_EXPERIMENTAL
INSTANTIATE_UTEST_SUITE_P(Driver, ExperimentalPool, ::testing::Values(true));
#endif

USERVER_NAMESPACE_END
