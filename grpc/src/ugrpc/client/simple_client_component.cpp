#include <userver/ugrpc/client/simple_client_component.hpp>

#include <userver/yaml_config/merge_schemas.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::client {

yaml_config::Schema SimpleClientComponentAny::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<components::LoggableComponentBase>(R"(
type: object
description: Provides a ClientFactory in the component system
additionalProperties: false
properties:
    endpoint:
        type: string
        description: URL of the gRPC service
    factory-component:
        type: string
        description: ClientFactoryComponent name to use for client creation
)");
}

}  // namespace ugrpc::client

USERVER_NAMESPACE_END
