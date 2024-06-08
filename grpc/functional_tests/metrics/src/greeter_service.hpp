#pragma once

#include <userver/components/component.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <samples/greeter_service.usrv.pb.hpp>

#include "greeter_client.hpp"

namespace functional_tests {

class GreeterServiceComponent final
    : public samples::api::GreeterServiceBase::Component {
 public:
  static constexpr std::string_view kName = "greeter-service";

  GreeterServiceComponent(const components::ComponentConfig& config,
                          const components::ComponentContext& context)
      : samples::api::GreeterServiceBase::Component(config, context),
        client_(context.FindComponent<GreeterClient>()) {}

  void SayHello(SayHelloCall& call,
                samples::api::GreetingRequest&& request) override;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  GreeterClient& client_;
};

inline void GreeterServiceComponent::SayHello(
    SayHelloCall& call, samples::api::GreetingRequest&& request) {
  samples::api::GreetingResponse response;
  const auto greeting = client_.SayHello(request.name());
  response.set_greeting("FWD: " + greeting);
  call.Finish(response);
}

inline yaml_config::Schema GreeterServiceComponent::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<ugrpc::server::ServiceComponentBase>(R"(
type: object
description: gRPC sample greater service component
additionalProperties: false
properties: {}
)");
}

}  // namespace functional_tests
