#include <userver/utest/using_namespace_userver.hpp>

#include <utility>

#include <fmt/format.h>

#include <userver/components/component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>

#include <userver/ugrpc/client/client_factory_component.hpp>
#include <userver/ugrpc/server/server_component.hpp>
#include <userver/ugrpc/server/service_component_base.hpp>

#include <samples/greeter_client.usrv.pb.hpp>
#include <samples/greeter_service.usrv.pb.hpp>

namespace samples {

class GreeterServiceComponent final
    : public api::GreeterServiceBase::Component {
 public:
  static constexpr std::string_view kName = "greeter-service";

  GreeterServiceComponent(const components::ComponentConfig& config,
                          const components::ComponentContext& context)
      : api::GreeterServiceBase::Component(config, context) {}

  void SayHello(SayHelloCall& call, api::GreetingRequest&& request) override;
};

void GreeterServiceComponent::SayHello(
    api::GreeterServiceBase::SayHelloCall& call,
    api::GreetingRequest&& request) {
  api::GreetingResponse response;
  response.set_greeting(fmt::format("Hello, {}!", request.name()));
  call.Finish(response);
}

}  // namespace samples

int main(int argc, char* argv[]) {
  const auto component_list =
      components::MinimalServerComponentList()
          .Append<components::TestsuiteSupport>()
          .Append<ugrpc::client::ClientFactoryComponent>()
          .Append<ugrpc::server::ServerComponent>()
          .Append<samples::GreeterServiceComponent>();
  return utils::DaemonMain(argc, argv, component_list);
}
