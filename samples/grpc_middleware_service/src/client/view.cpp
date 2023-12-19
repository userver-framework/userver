#include "view.hpp"

namespace samples::grpc::auth {

GreeterClient::GreeterClient(const components::ComponentConfig& config,
                             const components::ComponentContext& context)
    : LoggableComponentBase(config, context),
      client_factory_(
          context.FindComponent<ugrpc::client::ClientFactoryComponent>()
              .GetFactory()),
      client_(client_factory_.MakeClient<api::GreeterServiceClient>(
          "greeter", config["endpoint"].As<std::string>())) {}

std::string GreeterClient::SayHello(std::string name) {
  api::GreetingRequest request;
  request.set_name(std::move(name));

  auto context = std::make_unique<::grpc::ClientContext>();
  context->set_deadline(
      engine::Deadline::FromDuration(std::chrono::seconds{20}));

  auto stream = client_.SayHello(request, std::move(context));

  api::GreetingResponse response = stream.Finish();

  return std::move(*response.mutable_greeting());
}

yaml_config::Schema GreeterClient::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<components::LoggableComponentBase>(R"(
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

}  // namespace samples::grpc::auth
