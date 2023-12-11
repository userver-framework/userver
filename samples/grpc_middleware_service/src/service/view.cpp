#include "view.hpp"

#include <userver/components/minimal_server_component_list.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

namespace samples::grpc::auth {

GreeterServiceComponent::GreeterServiceComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : api::GreeterServiceBase::Component(config, context),
      prefix_(config["greeting-prefix"].As<std::string>()) {}

void GreeterServiceComponent::SayHello(
    api::GreeterServiceBase::SayHelloCall& call,
    api::GreetingRequest&& request) {
  api::GreetingResponse response;
  response.set_greeting(fmt::format("{}, {}!", prefix_, request.name()));

  call.Finish(response);
}

yaml_config::Schema GreeterServiceComponent::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<ugrpc::server::ServiceComponentBase>(R"(
type: object
description: gRPC sample greater service component
additionalProperties: false
properties:
    greeting-prefix:
        type: string
        description: greeting prefix
)");
}

}  // namespace samples::grpc::auth
