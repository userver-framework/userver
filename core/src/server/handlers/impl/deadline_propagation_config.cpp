#include <userver/server/handlers/impl/deadline_propagation_config.hpp>

#include <userver/dynamic_config/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::impl {

bool ParseDeadlinePropagationEnabled(const dynamic_config::DocsMap& docs_map) {
  return docs_map.Get("USERVER_DEADLINE_PROPAGATION_ENABLED").As<bool>();
}

}  // namespace server::handlers::impl

USERVER_NAMESPACE_END
