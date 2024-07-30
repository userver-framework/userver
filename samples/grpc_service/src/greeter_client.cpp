#include <greeter_client.hpp>

#include <chrono>
#include <memory>
#include <string>
#include <utility>

#include <userver/components/component.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/ugrpc/client/client_factory_component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

namespace samples {

// A user-defined wrapper around api::GreeterServiceClient that provides
// a simplified interface.
GreeterClient::GreeterClient(api::GreeterServiceClient&& raw_client)
    : raw_client_(std::move(raw_client)) {}

/// [client]
std::string GreeterClient::SayHello(std::string name) const {
  api::GreetingRequest request;
  request.set_name(std::move(name));

  // Deadline must be set manually for each RPC
  auto context = std::make_unique<grpc::ClientContext>();
  context->set_deadline(
      engine::Deadline::FromDuration(std::chrono::seconds{20}));

  // Initiate the RPC. No actual actions have been taken thus far besides
  // preparing to send the request.
  auto stream = raw_client_.SayHello(request, std::move(context));

  // Complete the unary RPC by sending the request and receiving the response.
  // The client should call `Finish` (in case of single response) or `Read`
  // until `false` (in case of response stream), otherwise the RPC will be
  // cancelled.
  api::GreetingResponse response = stream.Finish();

  return std::move(*response.mutable_greeting());
}
/// [client]

/// [component]
GreeterClientComponent::GreeterClientComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : ComponentBase(config, context),
      // ClientFactory is used to create gRPC clients
      client_factory_(
          context.FindComponent<ugrpc::client::ClientFactoryComponent>()
              .GetFactory()),
      // The client needs a fixed endpoint
      client_(client_factory_.MakeClient<api::GreeterServiceClient>(
          "greeter", config["endpoint"].As<std::string>())) {}
/// [component]

const GreeterClient& GreeterClientComponent::GetClient() const {
  return client_;
}

yaml_config::Schema GreeterClientComponent::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<components::ComponentBase>(R"(
type: object
description: >
    a user-defined wrapper around api::GreeterServiceClient that provides
    a simplified interface.
additionalProperties: false
properties:
    endpoint:
        type: string
        description: >
            the service endpoint (URI). We talk to our own service,
            which is kind of pointless, but works for an example
)");
}

}  // namespace samples
