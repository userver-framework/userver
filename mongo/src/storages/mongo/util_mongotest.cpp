#include <storages/mongo/util_mongotest.hpp>

#include <cstdlib>

#include <fmt/format.h>

#include <userver/clients/dns/resolver.hpp>
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

auto MakeDefaultPoolConfig() {
  storages::mongo::PoolConfig config;
  config.initial_size = 1;
  return config;
}

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
  return dynamic_config::StorageMock{
      {storages::mongo::kDefaultMaxTime, {}},
      {storages::mongo::kDeadlinePropagationEnabled, false}};
}

MongoPoolFixture::MongoPoolFixture()
    : default_resolver_(MakeDnsResolver()),
      dynamic_config_storage_(MakeDynamicConfig()),
      default_pool_(MakePool({}, {})) {}

MongoPoolFixture::~MongoPoolFixture() {
  const engine::TaskCancellationBlocker block_cancels;
  // TODO block task-inherited deadline via TaskCancellationBlocker?
  server::request::kTaskInheritedData.Erase();

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
  if (!config) config.emplace(MakeDefaultPoolConfig());
  if (!dns_resolver) dns_resolver.emplace(&default_resolver_);
  used_db_names_.insert(*db_name);
  return {*db_name, GetTestsuiteMongoUri(*db_name), *config, *dns_resolver,
          dynamic_config_storage_.GetSource()};
}

void MongoPoolFixture::SetDynamicConfig(
    const std::vector<dynamic_config::KeyValue>& config) {
  dynamic_config_storage_.Extend(config);
}

USERVER_NAMESPACE_END
