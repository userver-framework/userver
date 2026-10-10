#include <storages/mongo/util_mongotest.hpp>

#include <cstdlib>

#include <fmt/format.h>

#include <userver/clients/dns/resolver.hpp>
#include <userver/dynamic_config/test_helpers.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/request/task_inherited_data.hpp>

#include <userver/formats/bson/value.hpp>
#include <userver/storages/mongo/mongo_error.hpp>
#include <userver/storages/mongo/pool_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr const char* kTestsuiteMongosPort = "TESTSUITE_MONGOS_PORT";
constexpr const char* kDefaultMongoPort = "27217";

constexpr auto kTestConnTimeout = utest::kMaxTestWaitTime;
constexpr auto kTestSoTimeout = utest::kMaxTestWaitTime;
constexpr auto kTestQueueTimeout = std::chrono::milliseconds{10};
constexpr size_t kTestInitialSize = 1;
constexpr size_t kTestMaxSize = 16;
constexpr size_t kTestIdleLimit = 4;
constexpr size_t kTestConnectingLimit = 8;
constexpr auto kTestMaintenancePeriod = std::chrono::seconds{1};

void DropDatabase(storages::mongo::Pool& pool, const std::string& name) {
    LOG_INFO() << "Dropping database " << name << " after mongo tests";
    try {
        pool.DropDatabase();
    } catch (const std::exception& ex) {
        ADD_FAILURE() << "Error dropping mongo db after tests: " << ex.what();
    }
}

}  // namespace

const std::string kTestDatabaseNamePrefix = "userver_mongotest_";
const std::string kTestDatabaseDefaultName = "userver_mongotest_default";

std::string GetTestsuiteMongoUri(const std::string& database) {
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    const auto* mongo_port_env = std::getenv(kTestsuiteMongosPort);
    return fmt::format("mongodb://localhost:{}/{}", mongo_port_env ? mongo_port_env : kDefaultMongoPort, database);
}

clients::dns::Resolver MakeDnsResolver() {
    return clients::dns::Resolver{
        engine::current_task::GetTaskProcessor(),
        {},
    };
}

dynamic_config::StorageMock MakeDynamicConfig() { return dynamic_config::MakeDefaultStorage({}); }

storages::mongo::PoolConfig MakeTestPoolConfig() {
    storages::mongo::PoolConfig config;
    config.conn_timeout = kTestConnTimeout;
    config.so_timeout = kTestSoTimeout;
    config.queue_timeout = kTestQueueTimeout;
    config.pool_settings.initial_size = kTestInitialSize;
    config.pool_settings.max_size = kTestMaxSize;
    config.pool_settings.idle_limit = kTestIdleLimit;
    config.pool_settings.connecting_limit = kTestConnectingLimit;
    config.maintenance_period = kTestMaintenancePeriod;
    return config;
}

void ExpectWriteCounts(
    const storages::mongo::WriteResult& result,
    const ExpectedWriteCounts& expected,
    const utils::impl::SourceLocation& source_location
) {
    const auto source_location_string = utils::impl::ToString(source_location);
    EXPECT_EQ(expected.inserted, result.InsertedCount()) << " at " << source_location_string;
    EXPECT_EQ(expected.matched, result.MatchedCount()) << " at " << source_location_string;
    EXPECT_EQ(expected.modified, result.ModifiedCount()) << " at " << source_location_string;
    EXPECT_EQ(expected.upserted, result.UpsertedCount()) << " at " << source_location_string;
    EXPECT_EQ(expected.deleted, result.DeletedCount()) << " at " << source_location_string;
    EXPECT_EQ(expected.upserted, result.UpsertedIds().size()) << " at " << source_location_string;
}

void ExpectNoWriteErrors(
    const storages::mongo::WriteResult& result,
    const utils::impl::SourceLocation& source_location
) {
    const auto source_location_string = utils::impl::ToString(source_location);
    EXPECT_TRUE(result.ServerErrors().empty()) << " at " << source_location_string;
    EXPECT_TRUE(result.WriteConcernErrors().empty()) << " at " << source_location_string;
}

void ExpectSameWriteCounts(
    const storages::mongo::WriteResult& native,
    const storages::mongo::WriteResult& bulk_write,
    const utils::impl::SourceLocation& source_location
) {
    const auto source_location_string = utils::impl::ToString(source_location);
    EXPECT_EQ(native.InsertedCount(), bulk_write.InsertedCount()) << " at " << source_location_string;
    EXPECT_EQ(native.MatchedCount(), bulk_write.MatchedCount()) << " at " << source_location_string;
    EXPECT_EQ(native.ModifiedCount(), bulk_write.ModifiedCount()) << " at " << source_location_string;
    EXPECT_EQ(native.UpsertedCount(), bulk_write.UpsertedCount()) << " at " << source_location_string;
    EXPECT_EQ(native.DeletedCount(), bulk_write.DeletedCount()) << " at " << source_location_string;
}

void ExpectSingleUpsertedId(
    const storages::mongo::WriteResult& result,
    int expected_id,
    const utils::impl::SourceLocation& source_location
) {
    const auto source_location_string = utils::impl::ToString(source_location);
    const auto upserted_ids = result.UpsertedIds();
    ASSERT_EQ(std::size_t{1}, upserted_ids.size()) << " at " << source_location_string;
    const auto it = upserted_ids.find(0);
    ASSERT_NE(upserted_ids.end(), it) << " at " << source_location_string;
    ASSERT_TRUE(it->second.IsInt32()) << " at " << source_location_string;
    EXPECT_EQ(expected_id, it->second.As<int>()) << " at " << source_location_string;
}

void ExpectSingleDuplicateKeyError(
    const storages::mongo::WriteResult& result,
    const utils::impl::SourceLocation& source_location
) {
    const auto source_location_string = utils::impl::ToString(source_location);
    const auto server_errors = result.ServerErrors();
    ASSERT_EQ(std::size_t{1}, server_errors.size()) << " at " << source_location_string;
    const auto& error = server_errors.begin()->second;
    EXPECT_TRUE(error.IsServerError()) << " at " << source_location_string;
    EXPECT_EQ(kDuplicateKeyErrorCode, error.Code()) << " at " << source_location_string;
    EXPECT_EQ(storages::mongo::MongoError::Kind::kDuplicateKey, error.GetKind()) << " at " << source_location_string;
}

MongoPoolFixture::MongoPoolFixture()
    : default_resolver_(MakeDnsResolver()),
      dynamic_config_storage_(MakeDynamicConfig()),
      default_pool_(MakePool({}, {}))
{}

MongoPoolFixture::~MongoPoolFixture() {
    const engine::TaskCancellationBlocker block_cancels;
    const server::request::DeadlinePropagationBlocker block_dp;

    DropDatabase(default_pool_, kTestDatabaseDefaultName);
    used_db_names_.erase(kTestDatabaseDefaultName);

    for (const auto& db_name : used_db_names_) {
        if (db_name.starts_with(kTestDatabaseNamePrefix)) {
            auto pool = MakePool(db_name, {});
            DropDatabase(pool, db_name);
        }
    }
}

storages::mongo::Pool& MongoPoolFixture::GetDefaultPool() { return default_pool_; }

storages::mongo::Pool MongoPoolFixture::MakePool(
    std::optional<std::string> db_name,
    std::optional<storages::mongo::PoolConfig> config,
    std::optional<clients::dns::Resolver*> dns_resolver
) {
    if (!db_name) {
        db_name.emplace(kTestDatabaseDefaultName);
    }
    if (!config) {
        config.emplace(MakeTestPoolConfig());
    }
    if (!dns_resolver) {
        dns_resolver.emplace(&default_resolver_);
    }
    used_db_names_.insert(*db_name);
    storages::mongo::Pool
        pool{*db_name, GetTestsuiteMongoUri(*db_name), *config, *dns_resolver, dynamic_config_storage_.GetSource()};
    return pool;
}

storages::mongo::Pool MongoPoolFixture::MakeUnacknowledgedPool(const std::string& db_name) {
    used_db_names_.insert(db_name);
    return storages::mongo::Pool{
        db_name,
        GetTestsuiteMongoUri(db_name) + "?w=0",
        MakeTestPoolConfig(),
        &default_resolver_,
        dynamic_config_storage_.GetSource(),
    };
}

void MongoPoolFixture::SetDynamicConfig(const std::vector<dynamic_config::KeyValue>& config) {
    dynamic_config_storage_.Extend(config);
}

USERVER_NAMESPACE_END
