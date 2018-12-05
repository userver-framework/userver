#include <storages/mongo/pool_config.hpp>

#include <storages/mongo/error.hpp>

namespace storages {
namespace mongo {

PoolConfig::PoolConfig(const components::ComponentConfig& component_config)
    : conn_timeout_ms(
          component_config.ParseInt("conn_timeout_ms", kDefaultConnTimeoutMs)),
      so_timeout_ms(
          component_config.ParseInt("so_timeout_ms", kDefaultSoTimeoutMs)),
      min_size(component_config.ParseUint64("min_pool_size", kDefaultMinSize)),
      max_size(component_config.ParseUint64("max_pool_size", kDefaultMaxSize)) {
  if (conn_timeout_ms <= 0) {
    throw InvalidConfig("invalid conn_timeout");
  }
  if (so_timeout_ms <= 0) {
    throw InvalidConfig("invalid so_timeout");
  }
  if (!max_size || min_size > max_size) {
    throw InvalidConfig("invalid pool sizes");
  }
}

}  // namespace mongo
}  // namespace storages
