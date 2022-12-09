#include <storages/mongo/util_mongotest.hpp>

#include <cstdlib>

#include <fmt/format.h>

#include <userver/clients/dns/resolver.hpp>
#include <userver/engine/task/task.hpp>

#include <storages/mongo/dynamic_config.hpp>
#include <userver/storages/mongo/pool_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr const char* kTestsuiteMongosPort = "TESTSUITE_MONGOS_PORT";
constexpr const char* kDefaultMongoPort = "27217";

}  // namespace

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
  return dynamic_config::StorageMock{{storages::mongo::kDefaultMaxTime, {}}};
}

dynamic_config::Source GetDefaultDynamicConfig() {
  static const auto storage = MakeDynamicConfig();
  return storage.GetSource();
}

storages::mongo::Pool MakeTestsuiteMongoPool(
    const std::string& name, clients::dns::Resolver* dns_resolver,
    dynamic_config::Source config_source) {
  return MakeTestsuiteMongoPool(
      name,
      storages::mongo::PoolConfig{
          name, storages::mongo::PoolConfig::DriverImpl::kMongoCDriver},
      dns_resolver, config_source);
}

storages::mongo::Pool MakeTestsuiteMongoPool(
    const std::string& name, const storages::mongo::PoolConfig& config,
    clients::dns::Resolver* dns_resolver,
    dynamic_config::Source config_source) {
  return {name, GetTestsuiteMongoUri(name), config, dns_resolver,
          config_source};
}

USERVER_NAMESPACE_END
