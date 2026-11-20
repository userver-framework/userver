#include <storages/mongo/features.hpp>
#include <storages/mongo/util_mongotest.hpp>

#include <cstddef>
#include <exception>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <fmt/format.h>
#include <gmock/gmock.h>
#include <mongoc/mongoc.h>

#include <formats/bson/wrappers.hpp>
#include <storages/mongo/cdriver/pool_impl.hpp>
#include <userver/clients/dns/resolver.hpp>
#include <userver/dynamic_config/storage_mock.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/resource_scopes.hpp>

#include <userver/formats/bson.hpp>
#include <userver/storages/mongo/collection.hpp>
#include <userver/storages/mongo/exception.hpp>
#include <userver/storages/mongo/mongo_error.hpp>
#include <userver/storages/mongo/operators.hpp>
#include <userver/storages/mongo/transaction.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace bson = formats::bson;

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace storages::mongo;

class MongoTransaction : public MongoPoolFixture {};

namespace {

class MongoTransactionAdmin final {
public:
    explicit MongoTransactionAdmin(const std::string& uri)
        : resolver_(MakeDnsResolver()),
          config_(MakeDynamicConfig()),
          pool_(std::in_place, "transaction_test_admin", uri, MakeTestPoolConfig(), &resolver_, config_.GetSource())
    {}

    bson::Document Execute(const bson::Document& command) {
        auto client = pool_->Acquire();
        bson::impl::UninitializedBson reply;
        storages::mongo::MongoError error;
        const bson_t* native_command_ptr = command.GetBson().get();
        if (!mongoc_client_command_simple(
                client.get(),
                "admin",
                native_command_ptr,
                nullptr,
                reply.Get(),
                error.GetNative()
            ))
        {
            error.Throw("Error executing transaction test command");
        }
        return bson::Document(reply.Extract());
    }

private:
    clients::dns::Resolver resolver_;
    dynamic_config::StorageMock config_;
    utils::WithResourceScopes<storages::mongo::impl::cdriver::CDriverPoolImpl> pool_;
};

struct SecondaryInfo {
    std::string uri;
    std::size_t member_count;
};

SecondaryInfo GetSecondaryInfo() {
    MongoTransactionAdmin admin(GetTestsuiteMongoUri("admin"));
    auto hello = admin.Execute(bson::MakeDoc("hello", 1));
    if (hello["msg"].As<std::string>("") == "isdbgrid") {
        const auto shards = admin.Execute(bson::MakeDoc("listShards", 1));
        const auto host = shards["shards"][0]["host"].As<std::string>();
        const auto separator = host.find('/');
        if (separator == std::string::npos) {
            throw std::runtime_error("Transaction test requires a replica set shard");
        }
        MongoTransactionAdmin
            shard(fmt::format("mongodb://{}/admin?replicaSet={}", host.substr(separator + 1), host.substr(0, separator))
            );
        hello = shard.Execute(bson::MakeDoc("hello", 1));
    }

    const auto primary = hello["primary"].As<std::string>();
    const auto hosts = hello["hosts"].As<std::vector<std::string>>();
    for (const auto& host : hosts) {
        if (host != primary) {
            return {.uri = fmt::format("mongodb://{}/admin?directConnection=true", host), .member_count = hosts.size()};
        }
    }
    throw std::runtime_error("Transaction test requires a secondary");
}

class PausedMongoSecondary final {
public:
    PausedMongoSecondary()
        : secondary_(GetSecondaryInfo()),
          admin_(secondary_.uri)
    {
        admin_.Execute(bson::MakeDoc("fsync", 1, "lock", true));
    }

    ~PausedMongoSecondary() {
        const engine::TaskCancellationBlocker block_cancels;
        try {
            admin_.Execute(bson::MakeDoc("fsyncUnlock", 1));
        } catch (const std::exception& error) {
            ADD_FAILURE() << "Failed to unlock test secondary: " << error.what();
        }
    }

    std::string MakeCommitTimeoutUri(const std::string& database_name) const {
        constexpr int kWriteConcernTimeoutMs = 100;
        return fmt::format(
            "{}?w={}&wtimeoutMS={}",
            GetTestsuiteMongoUri(database_name),
            secondary_.member_count,
            kWriteConcernTimeoutMs
        );
    }

private:
    const SecondaryInfo secondary_;
    MongoTransactionAdmin admin_;
};

void ExpectUnknownCommitResult(storages::mongo::Transaction& transaction) {
    EXPECT_THAT(
        [&transaction] { transaction.Commit(); },
        ::testing::Throws<storages::mongo::TransactionException>(::testing::Property(
            &storages::mongo::TransactionException::GetType,
            storages::mongo::TransactionException::Type::kUnknownCommitResult
        ))
    );
}

}  // namespace

UTEST_F(MongoTransaction, ConnectionStringChangesDatabaseBeforeTransactionStarts) {
    const std::string k_other_database = kTestDatabaseNamePrefix + "pending_transaction_uri_reload";
    const std::string k_collection = "pending_transaction_uri_reload";
    auto old_pool = MakePool({}, {});
    auto new_pool = MakePool(k_other_database, {});
    auto pool = MakePool({}, {});
    old_pool.GetCollection(k_collection).InsertOne(bson::MakeDoc("_id", 1));
    new_pool.GetCollection(k_collection).InsertOne(bson::MakeDoc("_id", 2));

    auto transaction = pool.BeginTransaction();
    pool.SetConnectionString(GetTestsuiteMongoUri(k_other_database));
    auto collection = transaction.GetCollection(k_collection);
    EXPECT_EQ(1, collection.Count(bson::MakeDoc("_id", 2)));
    EXPECT_EQ(0, collection.Count(bson::MakeDoc("_id", 1)));
    collection.InsertOne(bson::MakeDoc("_id", 3));
    transaction.Commit();

    EXPECT_EQ(1, new_pool.GetCollection(k_collection).Count(bson::MakeDoc("_id", 3)));
    EXPECT_EQ(0, old_pool.GetCollection(k_collection).Count(bson::MakeDoc("_id", 3)));
}

UTEST_F(MongoTransaction, ConnectionStringKeepsDatabaseForActiveTransaction) {
    const std::string k_other_database = kTestDatabaseNamePrefix + "active_transaction_uri_reload";
    const std::string k_collection = "active_transaction_uri_reload";
    auto old_pool = MakePool({}, {});
    auto new_pool = MakePool(k_other_database, {});
    auto pool = MakePool({}, {});
    old_pool.GetCollection(k_collection).InsertOne(bson::MakeDoc("_id", 1));
    new_pool.GetCollection(k_collection).InsertOne(bson::MakeDoc("_id", 2));

    auto transaction = pool.BeginTransaction();
    auto collection = transaction.GetCollection(k_collection);
    ASSERT_EQ(1, collection.Count(bson::MakeDoc("_id", 1)));
    ASSERT_TRUE(transaction.IsActive());

    pool.SetConnectionString(GetTestsuiteMongoUri(k_other_database));
    EXPECT_EQ(1, pool.GetCollection(k_collection).Count(bson::MakeDoc("_id", 2)));
    EXPECT_EQ(0, pool.GetCollection(k_collection).Count(bson::MakeDoc("_id", 1)));
    EXPECT_EQ(1, collection.Count(bson::MakeDoc("_id", 1)));
    EXPECT_EQ(0, collection.Count(bson::MakeDoc("_id", 2)));
    collection.InsertOne(bson::MakeDoc("_id", 3));
    auto another_collection = transaction.GetCollection(k_collection);
    EXPECT_EQ(1, another_collection.Count(bson::MakeDoc("_id", 1)));
    EXPECT_EQ(0, another_collection.Count(bson::MakeDoc("_id", 2)));
    another_collection.InsertOne(bson::MakeDoc("_id", 4));
    transaction.Commit();

    for (const auto id : {3, 4}) {
        EXPECT_EQ(1, old_pool.GetCollection(k_collection).Count(bson::MakeDoc("_id", id)));
        EXPECT_EQ(0, new_pool.GetCollection(k_collection).Count(bson::MakeDoc("_id", id)));
    }
    EXPECT_EQ(1, pool.GetCollection(k_collection).Count(bson::MakeDoc("_id", 2)));
}

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

UTEST_F(MongoTransaction, CommitCanBeRetriedAfterWriteConcernFailure) {
    const auto database_name = kTestDatabaseNamePrefix + "commit_write_concern_failure";
    const std::string collection_name = "test_commit_write_concern_failure";
    auto observer_pool = MakePool(database_name, {});
    auto regular_collection = observer_pool.GetCollection(collection_name);
    regular_collection.InsertOne(bson::MakeDoc("_id", 1, "value", 0));

    const PausedMongoSecondary secondary;
    auto pool = MakePool(database_name, {});
    // Pausing one secondary makes this acknowledgement count time out at commit.
    pool.SetConnectionString(secondary.MakeCommitTimeoutUri(database_name));
    auto transaction = pool.BeginTransaction();
    auto collection = transaction.GetCollection(collection_name);
    collection.UpdateOne(bson::MakeDoc("_id", 1), bson::MakeDoc(operators::kInc, bson::MakeDoc("value", 1)));

    ExpectUnknownCommitResult(transaction);
    UEXPECT_NO_THROW(transaction.Commit());
    EXPECT_EQ(transaction.GetState(), Transaction::State::kCommitted);

    const auto document = regular_collection.FindOne(bson::MakeDoc("_id", 1));
    ASSERT_TRUE(document.has_value());
    EXPECT_EQ(document.value()["value"].As<int>(), 1);
}

UTEST_F(MongoTransaction, UnknownCommitIsInactiveAndRejectsCollectionOperations) {
    const auto database_name = kTestDatabaseNamePrefix + "inactive_unknown_commit";
    const std::string collection_name = "test_inactive_unknown_commit";
    auto pool = MakePool(database_name, {});
    pool.GetCollection(collection_name).InsertOne(bson::MakeDoc("_id", 1));

    const PausedMongoSecondary secondary;
    pool.SetConnectionString(secondary.MakeCommitTimeoutUri(database_name));
    auto transaction = pool.BeginTransaction();
    auto collection = transaction.GetCollection(collection_name);
    collection.UpdateOne(bson::MakeDoc("_id", 1), bson::MakeDoc(operators::kSet, bson::MakeDoc("value", 1)));
    ExpectUnknownCommitResult(transaction);

    EXPECT_EQ(transaction.GetState(), Transaction::State::kCommitUnknown);
    EXPECT_FALSE(transaction.IsActive());
    UEXPECT_THROW(collection.InsertOne(bson::MakeDoc("_id", 2)), MongoException);
    UEXPECT_THROW(transaction.GetCollection(collection_name), MongoException);
}

UTEST_F(MongoTransaction, UnknownCommitPinsConnectionUntilDestruction) {
    const auto database_name = kTestDatabaseNamePrefix + "destroy_unknown_commit";
    const std::string collection_name = "test_destroy_unknown_commit";
    auto observer_pool = MakePool(database_name, {});
    observer_pool.GetCollection(collection_name).InsertOne(bson::MakeDoc("_id", 1));

    const PausedMongoSecondary secondary;
    auto config = MakeTestPoolConfig();
    config.pool_settings.max_size = 1;
    config.pool_settings.idle_limit = 1;
    auto pool = MakePool(database_name, config);
    pool.SetConnectionString(secondary.MakeCommitTimeoutUri(database_name));

    std::optional<storages::mongo::Collection> collection;
    {
        auto transaction = pool.BeginTransaction();
        collection.emplace(transaction.GetCollection(collection_name));
        collection.value()
            .UpdateOne(bson::MakeDoc("_id", 1), bson::MakeDoc(operators::kSet, bson::MakeDoc("value", 1)));
        ExpectUnknownCommitResult(transaction);
        UEXPECT_THROW(pool.GetCollection(collection_name).FindOne({}), PoolOverloadException);
    }

    UEXPECT_NO_THROW(pool.GetCollection(collection_name).FindOne(bson::MakeDoc("_id", 1)));
}

UTEST_F(MongoTransaction, CollectionHandleIsInvalidAfterUnknownCommitDestruction) {
    const auto database_name = kTestDatabaseNamePrefix + "destroy_unknown_commit_collection";
    const std::string collection_name = "test_destroy_unknown_commit_collection";
    auto pool = MakePool(database_name, {});
    pool.GetCollection(collection_name).InsertOne(bson::MakeDoc("_id", 1));

    const PausedMongoSecondary secondary;
    pool.SetConnectionString(secondary.MakeCommitTimeoutUri(database_name));
    std::optional<storages::mongo::Collection> collection;
    {
        auto transaction = pool.BeginTransaction();
        collection.emplace(transaction.GetCollection(collection_name));
        collection.value()
            .UpdateOne(bson::MakeDoc("_id", 1), bson::MakeDoc(operators::kSet, bson::MakeDoc("value", 1)));
        ExpectUnknownCommitResult(transaction);
    }

    UEXPECT_THROW(collection.value().FindOne({}), MongoException);
}

UTEST_F(MongoTransaction, AbortUnknownCommitPreservesUnknownOutcome) {
    const auto database_name = kTestDatabaseNamePrefix + "release_unknown_commit";
    const std::string collection_name = "test_release_unknown_commit";
    auto observer_pool = MakePool(database_name, {});
    observer_pool.GetCollection(collection_name).InsertOne(bson::MakeDoc("_id", 1));

    const PausedMongoSecondary secondary;
    auto pool = MakePool(database_name, {});
    pool.SetConnectionString(secondary.MakeCommitTimeoutUri(database_name));
    auto transaction = pool.BeginTransaction();
    auto collection = transaction.GetCollection(collection_name);
    collection.UpdateOne(bson::MakeDoc("_id", 1), bson::MakeDoc(operators::kSet, bson::MakeDoc("value", 1)));
    ExpectUnknownCommitResult(transaction);

    transaction.Abort();
    transaction.Abort();
    EXPECT_EQ(transaction.GetState(), Transaction::State::kCommitUnknown);
    const auto document = observer_pool.GetCollection(collection_name).FindOne(bson::MakeDoc("_id", 1));
    ASSERT_TRUE(document.has_value());
    EXPECT_EQ(document.value()["value"].As<int>(), 1);
}

UTEST_F(MongoTransaction, AbortUnknownCommitReleasesSession) {
    const auto database_name = kTestDatabaseNamePrefix + "abort_unknown_commit_session";
    const std::string collection_name = "test_abort_unknown_commit_session";
    auto config = MakeTestPoolConfig();
    config.pool_settings.max_size = 1;
    config.pool_settings.idle_limit = 1;
    auto pool = MakePool(database_name, config);
    pool.GetCollection(collection_name).InsertOne(bson::MakeDoc("_id", 1));

    const PausedMongoSecondary secondary;
    pool.SetConnectionString(secondary.MakeCommitTimeoutUri(database_name));
    auto transaction = pool.BeginTransaction();
    auto collection = transaction.GetCollection(collection_name);
    collection.UpdateOne(bson::MakeDoc("_id", 1), bson::MakeDoc(operators::kSet, bson::MakeDoc("value", 1)));
    ExpectUnknownCommitResult(transaction);
    UEXPECT_THROW(pool.GetCollection(collection_name).FindOne({}), PoolOverloadException);

    transaction.Abort();

    UEXPECT_NO_THROW(pool.GetCollection(collection_name).FindOne(bson::MakeDoc("_id", 1)));
    UEXPECT_THROW(transaction.Commit(), MongoException);
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
    static constexpr std::size_t kDocCount = 5;
    std::vector<formats::bson::Document> docs;
    docs.reserve(kDocCount);
    for (std::size_t i = 0; i < kDocCount; ++i) {
        docs.push_back(bson::MakeDoc("name", fmt::format("user_{}", i), "age", 20 + i));
    }

    auto insert_result = collection.InsertMany(docs);
    EXPECT_EQ(insert_result.InsertedCount(), 5);

    // Update one document
    auto filter = bson::MakeDoc("name", "user_2");
    auto update = bson::MakeDoc(operators::kSet, bson::MakeDoc("age", 100));

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

#ifdef USERVER_FEATURE_MONGO_BULKWRITE
UTEST_F(MongoTransaction, ReplaceOneWithMaxServerTime) {
    static const std::string kCollectionName = "test_txn_replace_max_server_time";

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

UTEST_F(MongoTransaction, ReplaceOneWithMaxServerTimeRejectsWriteConcern) {
    static const std::string kCollectionName = "test_txn_replace_write_concern";

    auto regular_collection = GetDefaultPool().GetCollection(kCollectionName);
    try {
        regular_collection.Drop();
    } catch (const MongoException&) {
        // Ignore if collection doesn't exist
    }

    regular_collection.InsertOne(bson::MakeDoc("_id", 1, "x", 1));

    constexpr options::WriteConcern::Level kWriteConcerns[] = {
        options::WriteConcern::kMajority,
        options::WriteConcern::kUnacknowledged,
    };
    for (const auto write_concern : kWriteConcerns) {
        auto txn = GetDefaultPool().BeginTransaction();
        auto collection = txn.GetCollection(kCollectionName);

        UEXPECT_THROW(
            collection.ReplaceOne(
                bson::MakeDoc("_id", 1),
                bson::MakeDoc("x", 2),
                write_concern,
                options::MaxServerTime{utest::kMaxTestWaitTime}
            ),
            MongoException
        );
    }

    auto committed_doc = regular_collection.FindOne(bson::MakeDoc("_id", 1));
    ASSERT_TRUE(committed_doc);
    EXPECT_EQ((*committed_doc)["x"].As<int>(), 1);
}
#endif

UTEST_F(MongoTransaction, UpdateWithMaxServerTime) {
    static const std::string kCollectionName = "test_txn_update_max_server_time";
    const options::MaxServerTime k_max_server_time{utest::kMaxTestWaitTime};

    auto regular_collection = GetDefaultPool().GetCollection(kCollectionName);
    try {
        regular_collection.Drop();
    } catch (const MongoException&) {
        // Ignore if collection doesn't exist
    }

    regular_collection.InsertMany({bson::MakeDoc("_id", 1, "x", 1), bson::MakeDoc("_id", 2, "x", 1)});

    auto txn = GetDefaultPool().BeginTransaction();
    auto collection = txn.GetCollection(kCollectionName);

    {
        auto result = collection.UpdateOne(
            bson::MakeDoc("_id", 1),
            bson::MakeDoc(operators::kSet, bson::MakeDoc("x", 2)),
            k_max_server_time
        );
        ExpectWriteCounts(result, {.matched = 1, .modified = 1});
        ExpectNoWriteErrors(result);
    }

    auto uncommitted_doc = regular_collection.FindOne(bson::MakeDoc("_id", 1));
    ASSERT_TRUE(uncommitted_doc);
    EXPECT_EQ((*uncommitted_doc)["x"].As<int>(), 1);

    {
        auto result =
            collection
                .UpdateMany(bson::MakeDoc(), bson::MakeDoc(operators::kSet, bson::MakeDoc("y", 1)), k_max_server_time);
        EXPECT_EQ(result.MatchedCount(), 2);
        EXPECT_EQ(result.ModifiedCount(), 2);
    }

    {
        auto result = collection.UpdateOne(
            bson::MakeDoc("_id", 3),
            bson::MakeDoc(operators::kSet, bson::MakeDoc("x", 3)),
            options::Upsert{},
            k_max_server_time
        );
        ExpectWriteCounts(result, {.upserted = 1});
        ExpectSingleUpsertedId(result, 3);
    }

    txn.Commit();

    auto committed_doc = regular_collection.FindOne(bson::MakeDoc("_id", 1));
    ASSERT_TRUE(committed_doc);
    EXPECT_EQ((*committed_doc)["x"].As<int>(), 2);
    EXPECT_EQ((*committed_doc)["y"].As<int>(), 1);
    EXPECT_EQ(regular_collection.CountApprox(), 3);
}

UTEST_F(MongoTransaction, UpdateWithMaxServerTimeAbort) {
    static const std::string kCollectionName = "test_txn_update_max_server_time_abort";

    auto regular_collection = GetDefaultPool().GetCollection(kCollectionName);
    try {
        regular_collection.Drop();
    } catch (const MongoException&) {
        // Ignore if collection doesn't exist
    }

    regular_collection.InsertOne(bson::MakeDoc("_id", 1, "x", 1));

    auto txn = GetDefaultPool().BeginTransaction();
    auto collection = txn.GetCollection(kCollectionName);

    auto result = collection.UpdateOne(
        bson::MakeDoc("_id", 1),
        bson::MakeDoc(operators::kSet, bson::MakeDoc("x", 2)),
        options::MaxServerTime{utest::kMaxTestWaitTime}
    );
    EXPECT_EQ(result.ModifiedCount(), 1);

    txn.Abort();

    auto doc = regular_collection.FindOne(bson::MakeDoc("_id", 1));
    ASSERT_TRUE(doc);
    EXPECT_EQ((*doc)["x"].As<int>(), 1);
}

UTEST_F(MongoTransaction, UpdateWithMaxServerTimeRejectsWriteConcern) {
    static const std::string kCollectionName = "test_txn_update_max_server_time_wc";

    auto regular_collection = GetDefaultPool().GetCollection(kCollectionName);
    try {
        regular_collection.Drop();
    } catch (const MongoException&) {
        // Ignore if collection doesn't exist
    }

    regular_collection.InsertOne(bson::MakeDoc("_id", 1, "x", 1));

    constexpr options::WriteConcern::Level kWriteConcerns[] = {
        options::WriteConcern::kMajority,
        options::WriteConcern::kUnacknowledged,
    };
    for (const auto write_concern : kWriteConcerns) {
        auto txn = GetDefaultPool().BeginTransaction();
        auto collection = txn.GetCollection(kCollectionName);

        UEXPECT_THROW(
            collection.UpdateOne(
                bson::MakeDoc("_id", 1),
                bson::MakeDoc(operators::kSet, bson::MakeDoc("x", 2)),
                write_concern,
                options::MaxServerTime{utest::kMaxTestWaitTime}
            ),
            MongoException
        );
    }

    auto doc = regular_collection.FindOne(bson::MakeDoc("_id", 1));
    ASSERT_TRUE(doc);
    EXPECT_EQ((*doc)["x"].As<int>(), 1);
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

UTEST_F(MongoTransaction, Isolation) {
    static const std::string kCollectionName = "test_isolation";

    auto regular_collection = GetDefaultPool().GetCollection(kCollectionName);

    auto txn1 = GetDefaultPool().BeginTransaction();
    auto txn2 = GetDefaultPool().BeginTransaction();
    auto txn_collection1 = txn1.GetCollection(kCollectionName);
    auto txn_collection2 = txn2.GetCollection(kCollectionName);

    txn_collection1.InsertOne(bson::MakeDoc("name", "test_user", "age", 15));

    EXPECT_TRUE(txn_collection1.FindOne({}));
    EXPECT_FALSE(txn_collection2.FindOne({}));
    EXPECT_FALSE(regular_collection.FindOne({}));
    EXPECT_FALSE(regular_collection.Find({}).HasMore());

    txn1.Commit();

    // Collection changed in other transaction
    UEXPECT_THROW(txn_collection2.FindOne({}), MongoException);

    EXPECT_TRUE(regular_collection.FindOne({}));
    EXPECT_TRUE(regular_collection.Find({}).HasMore());
}

UTEST_F(MongoTransaction, InvalidSessionIdReproducer) {
    static const std::string kCollectionName = "test_invalid_session_id";

    auto config = MakeTestPoolConfig();
    config.pool_settings.initial_size = 1;
    config.pool_settings.max_size = 3;
    config.pool_settings.idle_limit = 2;

    auto pool = MakePool("userver_mongotest_invalid_session_id", config);

    auto coll = pool.GetCollection(kCollectionName);

    {
        std::vector<formats::bson::Document> docs;
        docs.reserve(10);
        for (int i = 0; i < 10; ++i) {
            docs.push_back(bson::MakeDoc("foo", "bar", "value", i));
        }
        coll.InsertMany(std::move(docs));
    }

    auto txn = pool.BeginTransaction();
    // GetCollection starts a transaction pinned to a dedicated client.
    auto coll_txn = txn.GetCollection(kCollectionName);

    // A non-transactional cursor on the same pool used to pick up the
    // transactional client and cause "Invalid sessionId" errors on subsequent
    // transactional ops. After the fix the transaction owns its client and
    // this interleaving is safe.
    auto cursor = coll.Find(bson::MakeDoc("foo", "bar"));

    coll_txn.InsertOne(bson::MakeDoc("foo", "bar", "value", 11));

    for (const auto& doc : cursor) {
        EXPECT_NE(doc["value"].As<int>(), 11);
    }

    txn.Commit();
}

UTEST_F(MongoTransaction, TransactionOwnsItsClient) {
    static const std::string kCollectionName = "test_txn_owns_client";

    auto config = MakeTestPoolConfig();
    config.pool_settings.initial_size = 1;
    config.pool_settings.max_size = 1;
    config.pool_settings.idle_limit = 1;

    auto pool = MakePool("userver_mongotest_txn_owns_client", config);

    auto regular_collection = pool.GetCollection(kCollectionName);
    regular_collection.InsertOne(bson::MakeDoc("foo", "bar"));

    auto txn = pool.BeginTransaction();
    auto coll_txn = txn.GetCollection(kCollectionName);

    UEXPECT_THROW(regular_collection.InsertOne(bson::MakeDoc("foo", "baz")), PoolOverloadException);

    txn.Commit();

    UEXPECT_NO_THROW(regular_collection.InsertOne(bson::MakeDoc("foo", "baz")));
}

UTEST_F(MongoTransaction, OperationAfterEndThrows) {
    {
        static const std::string kCollectionName = "test_ops_after_commit";

        auto regular_collection = GetDefaultPool().GetCollection(kCollectionName);

        auto txn = GetDefaultPool().BeginTransaction();
        auto coll_txn = txn.GetCollection(kCollectionName);
        coll_txn.InsertOne(bson::MakeDoc("foo", "bar"));
        txn.Commit();

        UEXPECT_THROW(coll_txn.InsertOne(bson::MakeDoc("foo", "bar")), MongoException);
        UEXPECT_THROW(txn.GetCollection(kCollectionName), MongoException);
        UEXPECT_THROW(txn.Commit(), MongoException);
    }
    {
        static const std::string kCollectionName = "test_ops_after_abort";

        auto regular_collection = GetDefaultPool().GetCollection(kCollectionName);

        auto txn = GetDefaultPool().BeginTransaction();
        auto coll_txn = txn.GetCollection(kCollectionName);
        coll_txn.InsertOne(bson::MakeDoc("foo", "bar"));

        txn.Abort();

        UEXPECT_THROW(coll_txn.InsertOne(bson::MakeDoc("foo", "bar")), MongoException);
        UEXPECT_THROW(txn.GetCollection(kCollectionName), MongoException);
        UEXPECT_THROW(coll_txn.FindOne({}), MongoException);
    }
}

#ifdef NDEBUG
UTEST_F(MongoTransaction, CursorOpsForbidden) {
    {
        static const std::string kCollectionName = "test_cursor_ops_forbidden";

        auto txn = GetDefaultPool().BeginTransaction();
        auto coll_txn = txn.GetCollection(kCollectionName);

        UEXPECT_THROW(coll_txn.Find(bson::MakeDoc("foo", "bar")), utils::InvariantError);
        UEXPECT_THROW(coll_txn.Aggregate(bson::MakeDoc("foo", "bar")), utils::InvariantError);
    }
}
#endif

USERVER_NAMESPACE_END
