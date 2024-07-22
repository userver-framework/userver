#include <greeter_service.hpp>

#include <fmt/format.h>

#include <userver/components/component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

namespace samples {

GreeterService::GreeterService(std::string prefix)
    : prefix_(std::move(prefix)) {}

/// [server RPC handling]
void GreeterService::SayHello(api::GreeterServiceBase::SayHelloCall& call,
                              api::GreetingRequest&& request) {
  // Authentication checking could have gone here.
  // Or even better, use a global gRPC authentication middleware.

  api::GreetingResponse response;
  response.set_greeting(fmt::format("{}, {}!", prefix_, request.name()));

  // Complete the RPC by sending the response. The service should complete
  // each request by calling `Finish` or `FinishWithError`, otherwise the
  // client will receive an Internal Error (500) response.
  call.Finish(response);
}
/// [server RPC handling]

/// [component]
GreeterServiceComponent::GreeterServiceComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : ugrpc::server::ServiceComponentBase(config, context),
      service_(config["greeting-prefix"].As<std::string>()) {
  RegisterService(service_);
}
/// [component]

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

}  // namespace samples
