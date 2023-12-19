#include <userver/server/handlers/impl/deadline_propagation_config.hpp>

#include <userver/dynamic_config/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::impl {

const dynamic_config::Key<bool> kDeadlinePropagationEnabled{
    "USERVER_DEADLINE_PROPAGATION_ENABLED", true};

}  // namespace server::handlers::impl

USERVER_NAMESPACE_END
