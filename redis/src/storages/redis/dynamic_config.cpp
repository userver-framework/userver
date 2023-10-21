#include "dynamic_config.hpp"

#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

const dynamic_config::Key<int> kDeadlinePropagationVersion{
    "REDIS_DEADLINE_PROPAGATION_VERSION", 1};

const dynamic_config::Key<bool> kRedisAutoTopologyEnabled{
    "REDIS_CLUSTER_AUTOTOPOLOGY_ENABLED_V2", true};

}  // namespace redis

USERVER_NAMESPACE_END
