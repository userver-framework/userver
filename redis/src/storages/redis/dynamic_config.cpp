#include "dynamic_config.hpp"

#include <userver/dynamic_config/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

int ParseDeadlinePropagationVersion(const dynamic_config::DocsMap& docs_map) {
  return docs_map.Get("REDIS_DEADLINE_PROPAGATION_VERSION").As<int>(0);
}

bool ParseRedisClusterAutoTopology(const dynamic_config::DocsMap& docs_map) {
  return docs_map.Get("REDIS_CLUSTER_AUTOTOPOLOGY_ENABLED").As<bool>(false);
}

}  // namespace redis

USERVER_NAMESPACE_END
