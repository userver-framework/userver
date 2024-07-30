#include <userver/storages/mongo/utest/mongo_local.hpp>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace storages::mongo::utest {

namespace {

constexpr const char* kTestsuiteMongosPort = "TESTSUITE_MONGOS_PORT";
constexpr const char* kDefaultMongoPort = "27217";

constexpr std::string_view kDefaultDatabaseName = "userver_mongotest_default";

std::string MakeConnectionString(std::string_view database) {
  // NOLINTNEXTLINE(concurrency-mt-unsafe)
  const auto* mongo_port_env = std::getenv(kTestsuiteMongosPort);
  return fmt::format("mongodb://localhost:{}/{}",
                     mongo_port_env ? mongo_port_env : kDefaultMongoPort,
                     database);
}

storages::mongo::PoolPtr CreatePool(std::string_view dbname,
                                    storages::mongo::PoolConfig pool_config,
                                    dynamic_config::Source config_source) {
  return std::make_shared<storages::mongo::Pool>(
      std::string{dbname}, MakeConnectionString(dbname), pool_config, nullptr,
      config_source);
}

}  // namespace

storages::mongo::PoolConfig MakeDefaultPoolConfig() {
  constexpr std::chrono::seconds kMaxTimeout{20};

  storages::mongo::PoolConfig config;
  config.conn_timeout = kMaxTimeout;
  config.so_timeout = kMaxTimeout;
  config.queue_timeout = std::chrono::milliseconds{10};
  config.pool_settings.initial_size = 1;
  config.pool_settings.max_size = 16;
  config.pool_settings.idle_limit = 4;
  config.pool_settings.connecting_limit = 8;
  config.maintenance_period = std::chrono::seconds{1};

  return config;
}

MongoLocal::MongoLocal()
    : MongoLocal(kDefaultDatabaseName, MakeDefaultPoolConfig()) {}

MongoLocal::MongoLocal(std::optional<std::string_view> dbname,
                       std::optional<storages::mongo::PoolConfig> pool_config,
                       dynamic_config::Source config_source)
    : pool_{CreatePool(dbname.value_or(kDefaultDatabaseName),
                       pool_config.value_or(MakeDefaultPoolConfig()),
                       config_source)} {}

storages::mongo::PoolPtr MongoLocal::GetPool() const { return pool_; }

}  // namespace storages::mongo::utest

USERVER_NAMESPACE_END
