#include <userver/utest/utest.hpp>

#include <storages/mongo/util_mongotest.hpp>
#include <userver/formats/bson.hpp>
#include <userver/storages/mongo.hpp>
#include <userver/storages/mongo/exception.hpp>
#include <userver/storages/mongo/operators.hpp>

USERVER_NAMESPACE_BEGIN

namespace bson = formats::bson;
namespace mongo = storages::mongo;
namespace bulk_ops = storages::mongo::bulk_ops;

namespace {
class Bulk : public MongoPoolFixture {};
}  // namespace

UTEST_F(Bulk, Empty) {
    auto coll = GetDefaultPool().GetCollection("empty");

    auto bulk = coll.MakeUnorderedBulk();
    EXPECT_TRUE(bulk.IsEmpty());
    auto result = coll.Execute(std::move(bulk));

    ExpectWriteCounts(result, {});
    ExpectNoWriteErrors(result);
}

UTEST_F(Bulk, DISABLED_InsertOne) {  // TODO: TAXICOMMON-8662
    auto coll = GetDefaultPool().GetCollection("insert_one");

    {
        auto bulk = coll.MakeOrderedBulk();
        bulk.InsertOne(bson::MakeDoc("x", 1));
        EXPECT_FALSE(bulk.IsEmpty());
        auto result = coll.Execute(std::move(bulk));

        ExpectWriteCounts(result, {.inserted = 1});
        ExpectNoWriteErrors(result);
    }
    {
        auto bulk = coll.MakeUnorderedBulk(mongo::options::WriteConcern::kMajority);
        bulk.InsertOne(bson::MakeDoc("x", 1));
        EXPECT_FALSE(bulk.IsEmpty());
        auto result = coll.Execute(std::move(bulk));

        ExpectWriteCounts(result, {.inserted = 1});
        ExpectNoWriteErrors(result);
    }
    {
        auto bulk = coll.MakeOrderedBulk();
        bulk.InsertOne(bson::MakeDoc("_id", 1));
        bulk.InsertOne(bson::MakeDoc("_id", 1));
        EXPECT_FALSE(bulk.IsEmpty());
        UEXPECT_THROW(coll.Execute(std::move(bulk)), mongo::DuplicateKeyException);
    }
    coll.DeleteMany({});
    {
        auto bulk = coll.MakeOrderedBulk(mongo::options::SuppressServerExceptions{});
        bulk.InsertOne(bson::MakeDoc("_id", 1));
        bulk.InsertOne(bson::MakeDoc("_id", 2));
        bulk.InsertOne(bson::MakeDoc("_id", 1));
        bulk.InsertOne(bson::MakeDoc("_id", 3));
        bulk.InsertOne(bson::MakeDoc("_id", 1));
        EXPECT_FALSE(bulk.IsEmpty());
        auto result = coll.Execute(std::move(bulk));

        ExpectWriteCounts(result, {.inserted = 2});
        EXPECT_TRUE(result.WriteConcernErrors().empty());
        const auto& operation_error = result.OperationError();

        EXPECT_TRUE(operation_error);
        EXPECT_EQ(kDuplicateKeyErrorCode, operation_error.Code());

        ExpectSingleDuplicateKeyError(result);
        EXPECT_EQ(1, result.ServerErrors().count(2));
    }
    coll.DeleteMany({});
    {
        auto bulk = coll.MakeUnorderedBulk(mongo::options::SuppressServerExceptions{});
        bulk.InsertOne(bson::MakeDoc("_id", 1));
        bulk.InsertOne(bson::MakeDoc("_id", 2));
        bulk.InsertOne(bson::MakeDoc("_id", 1));
        bulk.InsertOne(bson::MakeDoc("_id", 3));
        bulk.InsertOne(bson::MakeDoc("_id", 1));
        EXPECT_FALSE(bulk.IsEmpty());
        auto result = coll.Execute(std::move(bulk));

        ExpectWriteCounts(result, {.inserted = 3});
        EXPECT_TRUE(result.WriteConcernErrors().empty());

        const auto& operation_error = result.OperationError();

        EXPECT_TRUE(operation_error);
        EXPECT_EQ(kDuplicateKeyErrorCode, operation_error.Code());

        auto errors = result.ServerErrors();
        ASSERT_EQ(2, errors.size());
        EXPECT_EQ(kDuplicateKeyErrorCode, errors[2].Code());
        EXPECT_EQ(kDuplicateKeyErrorCode, errors[4].Code());
    }
}

UTEST_F(Bulk, ReplaceOne) {
    auto coll = GetDefaultPool().GetCollection("replace_one");

    coll.InsertOne(bson::MakeDoc("_id", 1));
    {
        auto bulk = coll.MakeOrderedBulk();
        bulk.ReplaceOne({}, bson::MakeDoc("x", 2));
        EXPECT_FALSE(bulk.IsEmpty());
        auto result = coll.Execute(std::move(bulk));

        ExpectWriteCounts(result, {.matched = 1, .modified = 1});
        ExpectNoWriteErrors(result);
    }
    {
        auto bulk =
            coll.MakeUnorderedBulk(mongo::options::WriteConcern::kMajority, mongo::options::SuppressServerExceptions{});
        bulk.ReplaceOne(bson::MakeDoc("y", 0), bson::MakeDoc("_id", 1), mongo::options::Upsert{});
        bulk.ReplaceOne({}, bson::MakeDoc("x", 3));
        EXPECT_FALSE(bulk.IsEmpty());
        auto result = coll.Execute(std::move(bulk));

        ExpectWriteCounts(result, {.matched = 1, .modified = 1});
        EXPECT_TRUE(result.WriteConcernErrors().empty());

        const auto& operation_error = result.OperationError();

        EXPECT_TRUE(operation_error);
        EXPECT_EQ(kDuplicateKeyErrorCode, operation_error.Code());
        ExpectSingleDuplicateKeyError(result);
    }
}

UTEST_F(Bulk, Update) {
    auto coll = GetDefaultPool().GetCollection("update");

    {
        auto bulk = coll.MakeOrderedBulk();
        bulk.UpdateOne(
            bson::MakeDoc("_id", 1),
            bson::MakeDoc(mongo::operators::kSet, bson::MakeDoc("x", 1)),
            mongo::options::Upsert{}
        );
        EXPECT_FALSE(bulk.IsEmpty());
        auto result = coll.Execute(std::move(bulk));

        ExpectWriteCounts(result, {.upserted = 1});
        ExpectNoWriteErrors(result);
        ExpectSingleUpsertedId(result, 1);
    }
    {
        auto bulk = coll.MakeUnorderedBulk(mongo::options::SuppressServerExceptions{});
        bulk.UpdateOne(
            bson::MakeDoc("y", 2),
            bson::MakeDoc(mongo::operators::kSetOnInsert, bson::MakeDoc("_id", 1)),
            mongo::options::Upsert{}
        );
        bulk.UpdateOne(
            bson::MakeDoc("_id", 2),
            bson::MakeDoc(mongo::operators::kSet, bson::MakeDoc("x", 2)),
            mongo::options::Upsert{}
        );
        bulk.UpdateMany({}, bson::MakeDoc(mongo::operators::kInc, bson::MakeDoc("x", 1)));
        EXPECT_FALSE(bulk.IsEmpty());
        auto result = coll.Execute(std::move(bulk));
        const auto& operation_error = result.OperationError();

        EXPECT_TRUE(operation_error);
        EXPECT_EQ(kDuplicateKeyErrorCode, operation_error.Code());

        ExpectWriteCounts(result, {.matched = 2, .modified = 2, .upserted = 1});
        EXPECT_TRUE(result.WriteConcernErrors().empty());

        ExpectSingleDuplicateKeyError(result);

        auto upserted_ids = result.UpsertedIds();
        EXPECT_EQ(2, upserted_ids[1].As<int>());
    }
    {
        const formats::bson::Value query =
            formats::bson::ValueBuilder(bson::MakeDoc(mongo::operators::kSet, bson::MakeDoc("z", 3))).ExtractValue();

        auto bulk = coll.MakeOrderedBulk();
        bulk.UpdateOne(bson::MakeDoc("_id", 1), query);  // Ensure Value overload correctly handles documents
        EXPECT_FALSE(bulk.IsEmpty());
        auto result = coll.Execute(std::move(bulk));

        ExpectWriteCounts(result, {.matched = 1, .modified = 1});
        ExpectNoWriteErrors(result);
    }
}

UTEST_F(Bulk, UpdateWithArrayFilters) {
    auto coll = GetDefaultPool().GetCollection("update_with_filter");
    {
        auto bulk = coll.MakeOrderedBulk();
        bulk.InsertOne(bson::MakeDoc("_id", 1, "x", 1, "array", bson::MakeArray(1, 2, 3)));
        bulk.InsertOne(bson::MakeDoc("_id", 2, "x", 2, "array", bson::MakeArray(3, 4, 5)));
        bulk.InsertOne(bson::MakeDoc("_id", 3, "x", 3, "array", bson::MakeArray(5, 6, 7)));

        EXPECT_FALSE(bulk.IsEmpty());
        auto result = coll.Execute(std::move(bulk));

        ExpectWriteCounts(result, {.inserted = 3});
        EXPECT_TRUE(result.ServerErrors().empty());
    }
    {
        auto bulk = coll.MakeOrderedBulk();
        auto options = mongo::options::ArrayFilters({bson::MakeDoc("elem", bson::MakeDoc(mongo::operators::kGte, 4))});

        bulk.UpdateMany(
            bson::MakeDoc(
                "array",
                bson::MakeDoc(mongo::operators::kElemMatch, bson::MakeDoc(mongo::operators::kGte, 4))
            ),
            bson::MakeDoc(mongo::operators::kSet, bson::MakeDoc("array.$[elem]", 10)),
            options
        );

        EXPECT_FALSE(bulk.IsEmpty());
        auto result = coll.Execute(std::move(bulk));

        ExpectWriteCounts(result, {.matched = 2, .modified = 2});
        ExpectNoWriteErrors(result);
    }
}

UTEST_F(Bulk, Delete) {
    auto coll = GetDefaultPool().GetCollection("delete");

    {
        std::vector<formats::bson::Document> docs;
        docs.reserve(10);
        for (int i = 0; i < 10; ++i) {
            docs.push_back(bson::MakeDoc("x", i));
        }
        coll.InsertMany(std::move(docs));
    }

    auto bulk = coll.MakeUnorderedBulk();
    bulk.DeleteOne(bson::MakeDoc("x", 1));
    bulk.DeleteOne(bson::MakeDoc("x", bson::MakeDoc(mongo::operators::kGt, 6)));
    bulk.DeleteMany(bson::MakeDoc("x", bson::MakeDoc(mongo::operators::kGt, 10)));
    bulk.DeleteMany(bson::MakeDoc("x", bson::MakeDoc(mongo::operators::kLt, 5)));
    EXPECT_FALSE(bulk.IsEmpty());
    auto result = coll.Execute(std::move(bulk));

    ExpectWriteCounts(result, {.deleted = 6});
    ExpectNoWriteErrors(result);
}

UTEST_F(Bulk, Mixed) {
    auto coll = GetDefaultPool().GetCollection("mixed");

    auto bulk = coll.MakeOrderedBulk();
    bulk.InsertOne(bson::MakeDoc("x", 1));
    bulk.InsertOne(bson::MakeDoc("x", 2));
    bulk.InsertOne(bson::MakeDoc("y", 3));
    bulk.UpdateMany(
        bson::MakeDoc("x", bson::MakeDoc(mongo::operators::kExists, true)),
        bson::MakeDoc(mongo::operators::kInc, bson::MakeDoc("x", -1))
    );
    bulk.ReplaceOne(bson::MakeDoc("y", 3), bson::MakeDoc("x", 2));
    bulk.UpdateOne(
        bson::MakeDoc("y", 3),
        bson::MakeDoc(mongo::operators::kSet, bson::MakeDoc("x", 3)),
        mongo::options::Upsert{}
    );
    bulk.DeleteMany(bson::MakeDoc("x", bson::MakeDoc(mongo::operators::kGt, 1)));
    bulk.UpdateMany({}, bson::MakeDoc(mongo::operators::kSet, bson::MakeDoc("x", 0)));
    bulk.DeleteOne(bson::MakeDoc("x", bson::MakeDoc(mongo::operators::kLt, 1)));
    EXPECT_FALSE(bulk.IsEmpty());
    auto result = coll.Execute(std::move(bulk));

    ExpectWriteCounts(result, {.inserted = 3, .matched = 5, .modified = 4, .upserted = 1, .deleted = 3});
    ExpectNoWriteErrors(result);

    auto upserted_ids = result.UpsertedIds();
    EXPECT_TRUE(upserted_ids[5].IsOid());
}

UTEST_F(Bulk, Hint) {
    auto coll = GetDefaultPool().GetCollection("hint");

    auto bulk = coll.MakeUnorderedBulk();
    bulk.InsertOne(bson::MakeDoc("_id", 1, "x", 1));
    bulk.UpdateOne(
        bson::MakeDoc("x", 1),
        bson::MakeDoc(mongo::operators::kSet, bson::MakeDoc("x", 2)),
        mongo::options::Hint{bson::MakeDoc("_id", 1)}
    );
    bulk.DeleteOne(bson::MakeDoc("x", 2), mongo::options::Hint{bson::MakeDoc("_id", 1)});

    UEXPECT_NO_THROW(coll.Execute(std::move(bulk)));
}

UTEST_F(Bulk, UpdateOneWithAggregationPipeline) {
    auto coll = GetDefaultPool().GetCollection("update_pipeline_one");
    coll.InsertOne(bson::MakeDoc("_id", 1, "x", 1));

    auto bulk = coll.MakeOrderedBulk();
    bulk.UpdateOne(
        bson::MakeDoc("_id", 1),
        bson::MakeArray(bson::MakeDoc(mongo::operators::kSet, bson::MakeDoc("x", 10)))
    );
    EXPECT_FALSE(bulk.IsEmpty());
    auto result = coll.Execute(std::move(bulk));

    ExpectWriteCounts(result, {.matched = 1, .modified = 1});
    ExpectNoWriteErrors(result);
}

UTEST_F(Bulk, UpdateManyWithAggregationPipeline) {
    auto coll = GetDefaultPool().GetCollection("update_pipeline_many");
    coll.InsertOne(bson::MakeDoc("_id", 1, "x", 1));
    coll.InsertOne(bson::MakeDoc("_id", 2, "x", 2));
    coll.InsertOne(bson::MakeDoc("_id", 3, "x", 3));

    auto bulk = coll.MakeOrderedBulk();
    bulk.UpdateMany(
        bson::MakeDoc("x", bson::MakeDoc(mongo::operators::kGt, 0)),
        bson::MakeArray(bson::MakeDoc(mongo::operators::kSet, bson::MakeDoc("updated", true)))
    );
    EXPECT_FALSE(bulk.IsEmpty());
    auto result = coll.Execute(std::move(bulk));

    ExpectWriteCounts(result, {.matched = 3, .modified = 3});
    ExpectNoWriteErrors(result);
}

UTEST_F(Bulk, UpdateWithMultiStageAggregationPipeline) {
    auto coll = GetDefaultPool().GetCollection("update_pipeline_multistage");
    coll.InsertOne(bson::MakeDoc("_id", 2, "x", 2));

    auto bulk = coll.MakeOrderedBulk();
    bulk.UpdateOne(
        bson::MakeDoc("_id", 2),
        bson::MakeArray(
            bson::MakeDoc(mongo::operators::kSet, bson::MakeDoc("y", 100)),
            bson::MakeDoc(mongo::operators::kUnset, "x")
        )
    );
    EXPECT_FALSE(bulk.IsEmpty());
    auto result = coll.Execute(std::move(bulk));

    ExpectWriteCounts(result, {.matched = 1, .modified = 1});
    EXPECT_TRUE(result.ServerErrors().empty());
}

UTEST_F(Bulk, UpdateWithAggregationPipelineInvalidType) {
    // Test: invalid update type (integer, not document or array) should throw on construction
    const bson::Value int_value = bson::ValueBuilder{42}.ExtractValue();
    UEXPECT_THROW(
        (bulk_ops::Update{bulk_ops::Update::Mode::kSingle, bson::MakeDoc("_id", 1), int_value}),
        mongo::InvalidQueryArgumentException
    );
}

USERVER_NAMESPACE_END
