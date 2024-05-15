#pragma once

#include <userver/components/component.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/ugrpc/client/client_factory_component.hpp>

#include <samples/greeter_client.usrv.pb.hpp>

namespace functional_tests {

class GreeterClient final : public components::LoggableComponentBase {
 public:
  static constexpr std::string_view kName = "greeter-client";

  GreeterClient(const components::ComponentConfig& config,
                const components::ComponentContext& context)
      : LoggableComponentBase(config, context),
        client_factory_(
            context.FindComponent<ugrpc::client::ClientFactoryComponent>()
                .GetFactory()),
        client_(client_factory_.MakeClient<samples::api::GreeterServiceClient>(
            "greeter", config["endpoint"].As<std::string>())) {}

  std::string SayHello(std::string name);

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  ugrpc::client::ClientFactory& client_factory_;
  samples::api::GreeterServiceClient client_;
};

inline yaml_config::Schema GreeterClient::GetStaticConfigSchema() {
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

inline std::string GreeterClient::SayHello(std::string name) {
  samples::api::GreetingRequest request;
  request.set_name(std::move(name));

  auto context = std::make_unique<grpc::ClientContext>();

  auto stream = client_.SayHello(request, std::move(context));

  samples::api::GreetingResponse response = stream.Finish();

  return std::move(*response.mutable_greeting());
}

}  // namespace functional_tests
