#pragma once

#include <userver/components/component.hpp>
#include <userver/server/request/task_inherited_request.hpp>
#include <userver/testsuite/tasks.hpp>
#include <userver/ugrpc/client/simple_client_component.hpp>
#include <userver/utest/using_namespace_userver.hpp>

#include <samples/greeter_client.usrv.pb.hpp>

namespace functional_tests {

using Client = samples::api::GreeterServiceClient;
using ClientComponent = ugrpc::client::SimpleClientComponent<Client>;

using ugrpc::client::ClientQos;

const dynamic_config::Key<ClientQos> kGreeterClientQos{"GREETER_CLIENT_QOS", ClientQos{}};

class GreeterClientComponent : public ClientComponent {
public:
    static constexpr std::string_view kName = "greeter-client";

    GreeterClientComponent(const components::ComponentConfig& config, const components::ComponentContext& context)
        : ClientComponent(config, context, kGreeterClientQos)
    {}
};

class GreeterClientTestComponent final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "greeter-client-test";

    GreeterClientTestComponent(const components::ComponentConfig& config, const components::ComponentContext& context)
        : components::ComponentBase(config, context),
          client_(context.FindComponent<GreeterClientComponent>().GetClient())
    {
        auto& tasks = testsuite::GetTestsuiteTasks(context);

        tasks.RegisterTask("call-say-hello", [this] { SayHello(); });
    }

private:
    static samples::api::GreetingRequest MakeGreetingRequest() {
        samples::api::GreetingRequest request;
        request.set_name("test");
        return request;
    }

    void SayHello() { [[maybe_unused]] const auto response = client_.SayHello(MakeGreetingRequest()); }

    Client& client_;
};

}  // namespace functional_tests
