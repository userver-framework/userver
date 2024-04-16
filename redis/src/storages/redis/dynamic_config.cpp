#include "dynamic_config.hpp"

#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

const dynamic_config::Key<int> kDeadlinePropagationVersion{
    "REDIS_DEADLINE_PROPAGATION_VERSION", 1};

}  // namespace redis

USERVER_NAMESPACE_END
