#include <storages/mongo/util_mongotest.hpp>

#include <cstdlib>

#include <fmt/format.h>

#include <userver/storages/mongo/pool_config.hpp>

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

storages::mongo::Pool MakeTestsuiteMongoPool(const std::string& name) {
  return MakeTestsuiteMongoPool(
      name, storages::mongo::PoolConfig{
                name, storages::mongo::PoolConfig::DriverImpl::kMongoCDriver});
}

storages::mongo::Pool MakeTestsuiteMongoPool(
    const std::string& name, const storages::mongo::PoolConfig& config) {
  return {name, GetTestsuiteMongoUri(name), config};
}
