#include <storages/mongo/util_mongotest.hpp>

#include <userver/formats/bson.hpp>
#include <userver/storages/mongo/exception.hpp>
#include <userver/storages/mongo/options.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace bson = formats::bson;

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace storages::mongo;

class Mongo5Transaction : public MongoPoolFixture {};

UTEST_F(Mongo5Transaction, TimedReplaceOneIsFirstOperation) {
    static const std::string kCollectionName = "test_txn_first_timed_replace";

    auto regular_collection = GetDefaultPool().GetCollection(kCollectionName);
    try {
        regular_collection.Drop();
    } catch (const MongoException&) {
        // Ignore if collection doesn't exist
    }
    regular_collection.InsertOne(bson::MakeDoc("_id", 1, "x", 1));

    auto txn = GetDefaultPool().BeginTransaction();
    auto collection = txn.GetCollection(kCollectionName);

    auto result = collection.ReplaceOne(
        bson::MakeDoc("_id", 1),
        bson::MakeDoc("x", 2),
        options::MaxServerTime{utest::kMaxTestWaitTime}
    );
    EXPECT_EQ(result.MatchedCount(), 1);
    EXPECT_EQ(result.ModifiedCount(), 1);

    auto uncommitted_doc = regular_collection.FindOne(bson::MakeDoc("_id", 1));
    ASSERT_TRUE(uncommitted_doc);
    EXPECT_EQ((*uncommitted_doc)["x"].As<int>(), 1);

    txn.Commit();

    auto committed_doc = regular_collection.FindOne(bson::MakeDoc("_id", 1));
    ASSERT_TRUE(committed_doc);
    EXPECT_EQ((*committed_doc)["x"].As<int>(), 2);
}

USERVER_NAMESPACE_END
