#include <userver/utest/using_namespace_userver.hpp>

#include <utility>

#include <fmt/format.h>

#include <userver/clients/dns/component.hpp>
#include <userver/clients/http/component.hpp>
#include <userver/components/component.hpp>
#include <userver/components/headers_propagator_component.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

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
      : api::GreeterServiceBase::Component(config, context),
        echo_url_{config["echo-url"].As<std::string>()},
        http_client_(
            context.FindComponent<components::HttpClient>().GetHttpClient()) {}

  void SayHello(SayHelloCall& call, api::GreetingRequest&& request) override;
  void CallEchoNobody(SayHelloCall& call,
                      api::GreetingRequest&& request) override;
  static yaml_config::Schema GetStaticConfigSchema() {
    return yaml_config::MergeSchemas<api::GreeterServiceBase::Component>(R"(
    type: object
    description: HTTP echo without body component
    additionalProperties: false
    properties:
        echo-url:
            type: string
            description: some other microservice listens on this URL
    )");
  }

 private:
  const std::string echo_url_;
  clients::http::Client& http_client_;
};

void GreeterServiceComponent::SayHello(
    api::GreeterServiceBase::SayHelloCall& call,
    api::GreetingRequest&& request) {
  api::GreetingResponse response;
  response.set_greeting(fmt::format("Hello, {}!", request.name()));
  call.Finish(response);
}

void GreeterServiceComponent::CallEchoNobody(
    api::GreeterServiceBase::SayHelloCall& call,
    samples::api::GreetingRequest&&) {
  api::GreetingResponse response;
  response.set_greeting("Call Echo Nobody");
  auto http_response = http_client_.CreateRequest()
                           .get(echo_url_)
                           .retry(1)
                           .timeout(std::chrono::seconds{5})
                           .perform();
  http_response->raise_for_status();
  call.Finish(response);
}

}  // namespace samples

int main(int argc, char* argv[]) {
  const auto component_list =
      components::MinimalServerComponentList()
          .Append<components::TestsuiteSupport>()
          .Append<ugrpc::client::ClientFactoryComponent>()
          .Append<ugrpc::server::ServerComponent>()
          .Append<samples::GreeterServiceComponent>()
          .Append<clients::dns::Component>()
          .Append<components::HeadersPropagatorComponent>()
          .Append<components::HttpClient>();
  return utils::DaemonMain(argc, argv, component_list);
}
