#include <userver/ugrpc/server/middlewares/log/component.hpp>

#include <ugrpc/server/middlewares/log/middleware.hpp>
#include <userver/components/component_config.hpp>
#include <userver/logging/level_serialization.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::log {

Component::Component(const components::ComponentConfig& config,
                     const components::ComponentContext& context)
    : MiddlewareComponentBase(config, context),
      settings_(config.As<Settings>()) {}

Component::~Component() = default;

std::shared_ptr<MiddlewareBase> Component::GetMiddleware() {
  return std::make_shared<Middleware>(*settings_);
}

yaml_config::Schema Component::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<MiddlewareComponentBase>(R"(
type: object
description: gRPC service logger component
additionalProperties: false
properties:
    log-level:
        type: string
        description: gRPC handlers log level
    msg-log-level:
        type: string
        description: set up log level for request/response messages body
    msg-size-log-limit:
        type: string
        description: max message size to log, the rest will be truncated
)");
}

}  // namespace ugrpc::server::middlewares::log

USERVER_NAMESPACE_END
