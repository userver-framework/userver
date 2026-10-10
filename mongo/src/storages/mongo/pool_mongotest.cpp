#include <userver/utest/utest.hpp>

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <storages/mongo/pool_impl.hpp>
#include <storages/mongo/util_mongotest.hpp>
#include <userver/clients/dns/resolver.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/formats/bson/document.hpp>
#include <userver/formats/bson/inline.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/storages/mongo/bulk.hpp>
#include <userver/storages/mongo/collection.hpp>
#include <userver/storages/mongo/cursor.hpp>
#include <userver/storages/mongo/exception.hpp>
#include <userver/storages/mongo/operators.hpp>
#include <userver/storages/mongo/options.hpp>
#include <userver/storages/mongo/pool.hpp>
#include <userver/storages/mongo/pool_config.hpp>

#include <dynamic_config/variables/MONGO_CONNECTION_POOL_SETTINGS.hpp>

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

    auto& pool = GetDefaultPool();
    auto coll = pool.GetCollection(kCollName);

    UEXPECT_NO_THROW(coll.InsertOne(formats::bson::MakeDoc("_id", 42)));
    EXPECT_TRUE(pool.HasCollection(kCollName));

    UEXPECT_NO_THROW(pool.DropDatabase());
    EXPECT_FALSE(pool.HasCollection(kCollName));

    UEXPECT_NO_THROW(coll.InsertOne(formats::bson::MakeDoc("_id", 42)));
    EXPECT_TRUE(pool.HasCollection(kCollName));
}

UTEST_F(Pool, ConnectionStringChangesDatabase) {
    using formats::bson::MakeDoc;
    const std::string k_other_database = kTestDatabaseNamePrefix + "uri_reload";
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

    pool.SetConnectionString(GetTestsuiteMongoUri(kTestDatabaseDefaultName));
    EXPECT_EQ(1, collection.Count({}));
}

UTEST_F(Pool, ConnectionStringChangesDatabaseWithMultipleConnections) {
    using formats::bson::MakeDoc;
    constexpr std::size_t k_initial_connections = 3;
    constexpr std::size_t k_busy_connections = 2;
    constexpr std::size_t k_max_connections = k_initial_connections + k_busy_connections;
    const std::string k_other_database = kTestDatabaseNamePrefix + "uri_reload_multiple_connections";
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

UTEST_F(Pool, InvalidConnectionStringKeepsDatabase) {
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

UTEST(NonexistentPool, ConnectionFailure) {
    auto dns_resolver = MakeDnsResolver();
    auto dynamic_config = MakeDynamicConfig();

    // constructor should not throw
    mongo::Pool bad_pool("bad", "mongodb://%2Fnonexistent.sock/bad", {}, &dns_resolver, dynamic_config.GetSource());
    UEXPECT_THROW(bad_pool.HasCollection("test"), mongo::ClusterUnavailableException);
}

UTEST_F(Pool, DynamicPoolSettingsDoesNotAbort) {
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

UTEST_F(Pool, Limits) {
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
}

UTEST_F(Pool, ListCollectionNames) {
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

UTEST_F(Pool, AggregateDocuments) {
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

UTEST_F(Pool, AggregateDocumentsGroup) {
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

UTEST_F(Pool, AggregateDocumentsLookup) {
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

UTEST_F(Pool, AggregateDocumentsOnCollectionRejected) {
    using formats::bson::MakeArray;
    using formats::bson::MakeDoc;

    auto coll = GetDefaultPool().GetCollection("aggregate_documents_rejected");
    UEXPECT_THROW(
        coll.Aggregate(MakeArray(MakeDoc(mongo::operators::kDocuments, MakeArray(MakeDoc("x", 1))))),
        mongo::MongoException
    );
}

UTEST_F(Pool, AggregateInvalidPipeline) {
    auto& pool = GetDefaultPool();
    UEXPECT_THROW(pool.Aggregate(formats::bson::MakeArray()), mongo::InvalidQueryArgumentException);
    UEXPECT_THROW(pool.Aggregate(formats::bson::MakeDoc("x", 1)), mongo::InvalidQueryArgumentException);
}

USERVER_NAMESPACE_END
