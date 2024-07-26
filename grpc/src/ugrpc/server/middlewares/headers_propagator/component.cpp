#include <userver/ugrpc/server/middlewares/headers_propagator/component.hpp>

#include <ugrpc/server/middlewares/headers_propagator/middleware.hpp>
#include <userver/components/component_config.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server::middlewares::headers_propagator {

Component::Component(const components::ComponentConfig& config,
                     const components::ComponentContext& context)
    : MiddlewareComponentBase(config, context), config_(config) {}

std::shared_ptr<MiddlewareBase> Component::GetMiddleware() {
  return std::make_shared<Middleware>(
      config_["headers"].As<std::vector<std::string>>({}));
}

yaml_config::Schema Component::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<MiddlewareComponentBase>(R"(
type: object
description: gRPC service metadata fields (headers) propagator middleware component
additionalProperties: false
properties:
    headers:
        type: array
        description: array of metadata fields(headers) to propagate
        items:
            type: string
            description: header
)");
}

}  // namespace ugrpc::server::middlewares::headers_propagator

USERVER_NAMESPACE_END
