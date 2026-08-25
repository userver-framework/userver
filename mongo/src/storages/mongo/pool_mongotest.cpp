#include <userver/utest/utest.hpp>

#include <chrono>
#include <string>
#include <string_view>
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
#include <userver/storages/mongo/operators.hpp>
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

    auto& pool = GetDefaultPool();
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
    mongo::Pool bad_pool("bad", "mongodb://%2Fnonexistent.sock/bad", {}, &dns_resolver, dynamic_config.GetSource());
    UEXPECT_THROW(bad_pool.HasCollection("test"), mongo::ClusterUnavailableException);
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
