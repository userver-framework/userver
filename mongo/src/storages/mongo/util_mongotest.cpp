#include <storages/mongo/util_mongotest.hpp>

#include <cstdlib>

#include <fmt/format.h>

#include <userver/clients/dns/resolver.hpp>
#include <userver/engine/task/task.hpp>
#include <userver/storages/mongo/pool_config.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr const char* kTestsuiteMongosPort = "TESTSUITE_MONGOS_PORT";
constexpr const char* kDefaultMongoPort = "27217";

}  // namespace

std::string GetTestsuiteMongoUri(const std::string& database) {
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

storages::mongo::Pool MakeTestsuiteMongoPool(
    const std::string& name, clients::dns::Resolver* dns_resolver,
    storages::mongo::Config mongo_config) {
  return MakeTestsuiteMongoPool(
      name,
      storages::mongo::PoolConfig{
          name, storages::mongo::PoolConfig::DriverImpl::kMongoCDriver},
      dns_resolver, mongo_config);
}

storages::mongo::Pool MakeTestsuiteMongoPool(
    const std::string& name, const storages::mongo::PoolConfig& config,
    clients::dns::Resolver* dns_resolver,
    storages::mongo::Config mongo_config) {
  return {name, GetTestsuiteMongoUri(name), config, dns_resolver, mongo_config};
}

USERVER_NAMESPACE_END
