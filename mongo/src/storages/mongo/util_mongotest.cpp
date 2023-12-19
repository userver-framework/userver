#include <storages/mongo/util_mongotest.hpp>

#include <cstdlib>

#include <fmt/format.h>

#include <userver/clients/dns/resolver.hpp>
#include <userver/dynamic_config/test_helpers.hpp>
#include <userver/engine/task/cancel.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/logging/log.hpp>
#include <userver/server/request/task_inherited_data.hpp>
#include <userver/utils/text.hpp>

#include <storages/mongo/dynamic_config.hpp>
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
  return fmt::format("mongodb://localhost:{}/{}",
                     mongo_port_env ? mongo_port_env : kDefaultMongoPort,
                     database);
}

clients::dns::Resolver MakeDnsResolver() {
  return clients::dns::Resolver{
      engine::current_task::GetTaskProcessor(),
      {},
  };
}

dynamic_config::StorageMock MakeDynamicConfig() {
  return dynamic_config::MakeDefaultStorage({});
}

storages::mongo::PoolConfig MakeTestPoolConfig() {
  storages::mongo::PoolConfig config;
  config.conn_timeout = kTestConnTimeout;
  config.so_timeout = kTestSoTimeout;
  config.queue_timeout = kTestQueueTimeout;
  config.initial_size = kTestInitialSize;
  config.max_size = kTestMaxSize;
  config.idle_limit = kTestIdleLimit;
  config.connecting_limit = kTestConnectingLimit;
  config.maintenance_period = kTestMaintenancePeriod;
  return config;
}

MongoPoolFixture::MongoPoolFixture()
    : default_resolver_(MakeDnsResolver()),
      dynamic_config_storage_(MakeDynamicConfig()),
      default_pool_(MakePool({}, {})) {}

MongoPoolFixture::~MongoPoolFixture() {
  const engine::TaskCancellationBlocker block_cancels;
  const server::request::DeadlinePropagationBlocker block_dp;

  DropDatabase(default_pool_, kTestDatabaseDefaultName);
  used_db_names_.erase(kTestDatabaseDefaultName);

  for (const auto& db_name : used_db_names_) {
    if (utils::text::StartsWith(db_name, kTestDatabaseNamePrefix)) {
      auto pool = MakePool(db_name, {});
      DropDatabase(pool, db_name);
    }
  }
}

storages::mongo::Pool MongoPoolFixture::GetDefaultPool() {
  return default_pool_;
}

storages::mongo::Pool MongoPoolFixture::MakePool(
    std::optional<std::string> db_name,
    std::optional<storages::mongo::PoolConfig> config,
    std::optional<clients::dns::Resolver*> dns_resolver) {
  if (!db_name) db_name.emplace(kTestDatabaseDefaultName);
  if (!config) config.emplace(MakeTestPoolConfig());
  if (!dns_resolver) dns_resolver.emplace(&default_resolver_);
  used_db_names_.insert(*db_name);
  storages::mongo::Pool pool{*db_name, GetTestsuiteMongoUri(*db_name), *config,
                             *dns_resolver,
                             dynamic_config_storage_.GetSource()};
  pool.Start();
  return pool;
}

void MongoPoolFixture::SetDynamicConfig(
    const std::vector<dynamic_config::KeyValue>& config) {
  dynamic_config_storage_.Extend(config);
}

USERVER_NAMESPACE_END
