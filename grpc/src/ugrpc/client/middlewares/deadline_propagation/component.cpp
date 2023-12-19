#include <userver/ugrpc/client/middlewares/deadline_propagation/component.hpp>

#include <ugrpc/client/middlewares/deadline_propagation/middleware.hpp>
#include <userver/components/component_config.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client::middlewares::deadline_propagation {

Component::Component(const components::ComponentConfig& config,
                     const components::ComponentContext& context)
    : MiddlewareComponentBase(config, context) {}

std::shared_ptr<const MiddlewareFactoryBase> Component::GetMiddlewareFactory() {
  return std::make_shared<MiddlewareFactory>();
}

yaml_config::Schema Component::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<MiddlewareComponentBase>(R"(
type: object
description: gRPC client deadline propagation middleware component
additionalProperties: false
properties: {}
)");
}

}  // namespace ugrpc::client::middlewares::deadline_propagation

USERVER_NAMESPACE_END
