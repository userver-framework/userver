#include <storages/mongo/util_mongotest.hpp>

#include <fmt/format.h>
#include <gmock/gmock.h>
#include <userver/utils/algo.hpp>

#include <userver/formats/bson.hpp>
#include <userver/storages/mongo/exception.hpp>
#include <userver/storages/mongo/transaction.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace bson = formats::bson;

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace storages::mongo;

class MongoTransaction : public MongoPoolFixture {};

UTEST_F(MongoTransaction, BasicTransactionCommit) {
    static const std::string kCollectionName = "test_transactions";

    // Clean up collection before test
    auto regular_collection = GetDefaultPool().GetCollection(kCollectionName);
    try {
        regular_collection.Drop();
    } catch (const MongoException&) {
        // Ignore if collection doesn't exist
    }

    auto txn = GetDefaultPool().BeginTransaction();
    auto collection = txn.GetCollection(kCollectionName);

    EXPECT_EQ(txn.GetState(), storages::mongo::Transaction::State::kInProgress);
    EXPECT_TRUE(txn.IsActive());

    // Insert document within transaction
    auto doc = bson::MakeDoc("name", "test_user", "age", 30);
    auto result = collection.InsertOne(doc);

    EXPECT_EQ(result.InsertedCount(), 1);

    // Commit transaction
    txn.Commit();

    EXPECT_EQ(txn.GetState(), storages::mongo::Transaction::State::kCommitted);
    EXPECT_FALSE(txn.IsActive());

    // Verify document exists after commit
    auto found_doc = regular_collection.FindOne({});
    EXPECT_TRUE(found_doc);
    EXPECT_EQ((*found_doc)["name"].As<std::string>(), "test_user");
}

UTEST_F(MongoTransaction, TransactionAbort) {
    static const std::string kCollectionName = "test_transaction_abort";

    // Clean up collection before test
    auto regular_collection = GetDefaultPool().GetCollection(kCollectionName);
    try {
        regular_collection.Drop();
    } catch (const MongoException&) {
        // Ignore if collection doesn't exist
    }

    auto txn = GetDefaultPool().BeginTransaction();
    auto collection = txn.GetCollection(kCollectionName);

    EXPECT_EQ(txn.GetState(), storages::mongo::Transaction::State::kInProgress);

    // Insert document within transaction
    auto doc = bson::MakeDoc("name", "test_user_to_abort", "age", 25);
    auto result = collection.InsertOne(doc);

    EXPECT_EQ(result.InsertedCount(), 1);

    // Abort transaction
    txn.Abort();

    EXPECT_EQ(txn.GetState(), storages::mongo::Transaction::State::kAborted);
    EXPECT_FALSE(txn.IsActive());

    // Verify document does not exist after abort
    auto found_doc = regular_collection.FindOne({});
    EXPECT_FALSE(found_doc);
}

UTEST_F(MongoTransaction, TransactionAutoAbortOnDestruction) {
    static const std::string kCollectionName = "test_auto_abort";

    // Clean up collection before test
    auto regular_collection = GetDefaultPool().GetCollection(kCollectionName);
    try {
        regular_collection.Drop();
    } catch (const MongoException&) {
        // Ignore if collection doesn't exist
    }

    {
        auto txn = GetDefaultPool().BeginTransaction();
        auto collection = txn.GetCollection(kCollectionName);

        // Insert document within transaction
        auto doc = bson::MakeDoc("name", "test_auto_abort", "age", 35);
        collection.InsertOne(doc);

        EXPECT_TRUE(txn.IsActive());
        // Transaction will be auto-aborted on scope exit
    }

    // Verify document does not exist after auto-abort
    auto found_doc = regular_collection.FindOne({});
    EXPECT_FALSE(found_doc);
}

UTEST_F(MongoTransaction, MultipleOperationsInTransaction) {
    static const std::string kCollectionName = "test_multi_ops";

    // Clean up collection before test
    auto regular_collection = GetDefaultPool().GetCollection(kCollectionName);
    try {
        regular_collection.Drop();
    } catch (const MongoException&) {
        // Ignore if collection doesn't exist
    }

    /// [transaction]
    auto txn = GetDefaultPool().BeginTransaction();
    auto collection = txn.GetCollection(kCollectionName);

    // Insert multiple documents
    std::vector<formats::bson::Document> docs;
    for (int i = 0; i < 5; ++i) {
        docs.push_back(bson::MakeDoc("name", fmt::format("user_{}", i), "age", 20 + i));
    }

    auto insert_result = collection.InsertMany(docs);
    EXPECT_EQ(insert_result.InsertedCount(), 5);

    // Update one document
    auto filter = bson::MakeDoc("name", "user_2");
    auto update = bson::MakeDoc("$set", bson::MakeDoc("age", 100));

    auto update_result = collection.UpdateOne(filter, update);
    EXPECT_EQ(update_result.ModifiedCount(), 1);

    // Delete one document
    auto delete_filter = bson::MakeDoc("name", "user_4");
    auto delete_result = collection.DeleteOne(delete_filter);
    EXPECT_EQ(delete_result.DeletedCount(), 1);

    // Commit all operations
    txn.Commit();
    /// [transaction]

    // Verify final state
    auto count = regular_collection.CountApprox();
    EXPECT_EQ(count, 4);  // 5 inserted - 1 deleted = 4

    // Verify update was applied
    auto updated_doc = regular_collection.FindOne(filter);
    EXPECT_TRUE(updated_doc);
    EXPECT_EQ((*updated_doc)["age"].As<int>(), 100);

    // Verify deleted document is gone
    auto deleted_doc = regular_collection.FindOne(delete_filter);
    EXPECT_FALSE(deleted_doc);
}

UTEST_F(MongoTransaction, Move) {
    static const std::string kCollectionName = "test_transactions";

    // Clean up collection before test
    auto regular_collection = GetDefaultPool().GetCollection(kCollectionName);
    try {
        regular_collection.Drop();
    } catch (const MongoException&) {
        // Ignore if collection doesn't exist
    }

    Transaction txn;
    {
        auto txn_tmp = GetDefaultPool().BeginTransaction();
        txn = std::move(txn_tmp);
    }
    auto collection = txn.GetCollection(kCollectionName);

    EXPECT_EQ(txn.GetState(), storages::mongo::Transaction::State::kInProgress);
    EXPECT_TRUE(txn.IsActive());

    // Insert document within transaction
    auto doc = bson::MakeDoc("name", "test_user", "age", 30);
    auto result = collection.InsertOne(doc);

    EXPECT_EQ(result.InsertedCount(), 1);

    // Commit transaction
    txn.Commit();

    EXPECT_EQ(txn.GetState(), storages::mongo::Transaction::State::kCommitted);
    EXPECT_FALSE(txn.IsActive());

    // Verify document exists after commit
    auto found_doc = regular_collection.FindOne({});
    EXPECT_TRUE(found_doc);
    EXPECT_EQ((*found_doc)["name"].As<std::string>(), "test_user");
}

UTEST_F(MongoTransaction, InvalidStateOperations) {
    static const std::string kCollectionName = "test_invalid_state";

    // Clean up collection before test
    auto regular_collection = GetDefaultPool().GetCollection(kCollectionName);
    try {
        regular_collection.Drop();
    } catch (const MongoException&) {
        // Ignore if collection doesn't exist
    }

    auto txn = GetDefaultPool().BeginTransaction();
    auto collection = txn.GetCollection(kCollectionName);

    // Commit transaction
    txn.Commit();

    EXPECT_FALSE(txn.IsActive());

    // Operations after commit should throw
    UEXPECT_THROW(txn.Commit(), MongoException);

    auto doc = bson::MakeDoc("name", "should_fail");

    // This should throw because transaction is not active
    UEXPECT_THROW(collection.InsertOne(doc), MongoException);
}

UTEST_F(MongoTransaction, ParallelTrancactions) {
    static const std::string kCollectionName = "test_parallel_transactions";

    // Clean up collection before test
    auto regular_collection = GetDefaultPool().GetCollection(kCollectionName);
    try {
        regular_collection.Drop();
    } catch (const MongoException&) {
        // Ignore if collection doesn't exist
    }

    {
        auto txn2 = GetDefaultPool().BeginTransaction();
        auto collection2 = txn2.GetCollection(kCollectionName);
        {
            auto txn1 = GetDefaultPool().BeginTransaction();
            auto collection1 = txn1.GetCollection(kCollectionName);

            auto doc = bson::MakeDoc("name", "test_user", "age", 30);
            collection1.InsertOne(doc);

            doc = bson::MakeDoc("name", "test_user2", "age", 31);
            collection2.InsertOne(doc);

            txn1.Commit();
        }
        UEXPECT_THROW(txn2.Commit(), MongoException);
    }

    // Verify document exists after commit
    options::Projection projection;
    projection.Exclude("_id");
    auto found_docs = utils::AsContainer<std::vector<formats::bson::Document>>(regular_collection.Find({}, projection));
    EXPECT_THAT(found_docs, ::testing::ElementsAre(bson::MakeDoc("name", "test_user", "age", 30)));
}

USERVER_NAMESPACE_END
