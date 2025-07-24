#pragma once

#include <userver/components/component.hpp>
#include <userver/server/request/task_inherited_request.hpp>
#include <userver/testsuite/tasks.hpp>
#include <userver/utest/using_namespace_userver.hpp>

#include <userver/ugrpc/client/simple_client_component.hpp>

#include <samples/greeter_client.usrv.pb.hpp>

namespace functional_tests {

using Client = samples::api::GreeterServiceClient;
using ClientComponent = ugrpc::client::SimpleClientComponent<Client>;

class GreeterClientTestComponent final : public components::ComponentBase {
public:
    static constexpr std::string_view kName = "greeter-client-test";

    GreeterClientTestComponent(const components::ComponentConfig& config, const components::ComponentContext& context)
        : components::ComponentBase(config, context),
          client_(context.FindComponent<ClientComponent>("greeter-client").GetClient()) {
        auto& tasks = testsuite::GetTestsuiteTasks(context);

        tasks.RegisterTask("call-say-hello", [this] { SayHello(); });

        tasks.RegisterTask("call-say-hello-headers-propagator", [this] {
            const server::request::HeadersToPropagate headers{
                {"skipped-header", "v1"},
                {"allowed-header", "v2"},
                {"X-Upper-Case-Header", "v3"},
            };
            server::request::SetPropagatedHeaders(headers);

            SayHello();
        });
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
