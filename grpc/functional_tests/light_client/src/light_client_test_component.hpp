#pragma once

#include <userver/components/component.hpp>
#include <userver/testsuite/tasks.hpp>
#include <userver/utest/using_namespace_userver.hpp>

#include <userver/ugrpc/client/simple_client_component.hpp>

#include <samples/greeter_client.usrv.pb.hpp>

namespace functional_tests {

using Client = samples::api::GreeterServiceClient;
using ClientComponent = ugrpc::client::SimpleClientComponent<Client>;

/// Exercises both a regular and a "light" gRPC client factory via testsuite
/// tasks. The "light" client is backed by `light-client-factory`, which
/// never blocks on `components::DynamicConfig` at construction time --
/// mimicking a client a service would use to deliver dynamic configs to
/// itself -- while the regular one is a plain everyday client.
class LightClientTestComponent final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "light-client-test";

    LightClientTestComponent(const components::ComponentConfig& config, const components::ComponentContext& context)
        : components::ComponentBase(config, context),
          regular_client_(context.FindComponent<ClientComponent>("regular-greeter-client").GetClient()),
          light_client_(context.FindComponent<ClientComponent>("light-greeter-client").GetClient())
    {
        auto& tasks = testsuite::GetTestsuiteTasks(context);
        tasks.RegisterTask("call-say-hello-regular", [this] { SayHello(regular_client_); });
        tasks.RegisterTask("call-say-hello-light", [this] { SayHello(light_client_); });
    }

private:
    static void SayHello(Client& client) {
        samples::api::GreetingRequest request;
        request.set_name("test");
        [[maybe_unused]] const auto response = client.SayHello(request);
    }

    Client& regular_client_;
    Client& light_client_;
};

}  // namespace functional_tests
