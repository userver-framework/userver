#include "dynamic_config.hpp"

#include <userver/dynamic_config/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

bool ParseDeadlinePropagationEnabled(const dynamic_config::DocsMap& docs_map) {
  return docs_map.Get("REDIS_DEADLINE_PROPAGATION_ENABLED").As<bool>(false);
}

bool ParseRedisClusterAutoTopology(const dynamic_config::DocsMap& docs_map) {
  return docs_map.Get("REDIS_CLUSTER_AUTOTOPOLOGY_ENABLED").As<bool>(false);
}

}  // namespace redis

USERVER_NAMESPACE_END
