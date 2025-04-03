#include "dynamic_config.hpp"

#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

const dynamic_config::Key<int> kDeadlinePropagationVersion{"REDIS_DEADLINE_PROPAGATION_VERSION", 1};

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
