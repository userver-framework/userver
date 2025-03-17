#pragma once

#include <userver/components/component.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/ugrpc/client/client_factory_component.hpp>
#include <userver/ugrpc/client/exceptions.hpp>
#include <userver/ugrpc/client/simple_client_component.hpp>

#include <samples/greeter_client.usrv.pb.hpp>
#include <samples/greeter_service.usrv.pb.hpp>

namespace functional_tests {

using Client = samples::api::GreeterServiceClient;
using ClientComponent = ugrpc::client::SimpleClientComponent<Client>;

class GreeterServiceComponent final : public samples::api::GreeterServiceBase::Component {
public:
    static constexpr std::string_view kName = "greeter-service";

    GreeterServiceComponent(const components::ComponentConfig& config, const components::ComponentContext& context)
        : samples::api::GreeterServiceBase::Component(config, context),
          client_factory_(context.FindComponent<ugrpc::client::ClientFactoryComponent>().GetFactory()),
          client_(context.FindComponent<ClientComponent>("greeter-client").GetClient()) {}

    SayHelloResult SayHello(CallContext& context, samples::api::GreetingRequest&& request) override;

    static yaml_config::Schema GetStaticConfigSchema();

private:
    ugrpc::client::ClientFactory& client_factory_;
    Client& client_;
};

inline GreeterServiceComponent::SayHelloResult
GreeterServiceComponent::SayHello(CallContext& /*context*/, samples::api::GreetingRequest&& request) {
    samples::api::GreetingResponse response;
    auto context = std::make_unique<grpc::ClientContext>();
    try {
        response.set_greeting(client_.SayHello(request, std::move(context)).greeting());
    } catch (const ugrpc::client::RpcInterruptedError& e) {
        response.set_greeting(fmt::format("Client caught mocked error: {}", e.what()));
    }

    return response;
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
