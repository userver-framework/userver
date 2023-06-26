#include <userver/utest/using_namespace_userver.hpp>

#include <fmt/format.h>

#include <userver/components/component.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/server/handlers/server_monitor.hpp>
#include <userver/testsuite/testsuite_support.hpp>
#include <userver/utils/daemon_run.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/ugrpc/client/client_factory_component.hpp>
#include <userver/ugrpc/client/middlewares/log/component.hpp>
#include <userver/ugrpc/server/middlewares/deadline_propagation/component.hpp>
#include <userver/ugrpc/server/middlewares/log/component.hpp>
#include <userver/ugrpc/server/server_component.hpp>
#include <userver/ugrpc/server/service_component_base.hpp>

#include <samples/greeter_client.usrv.pb.hpp>
#include <samples/greeter_service.usrv.pb.hpp>

namespace functional_tests {

class GreeterServiceComponent final
    : public samples::api::GreeterServiceBase::Component {
 public:
  static constexpr std::string_view kName = "greeter-service";

  GreeterServiceComponent(const components::ComponentConfig& config,
                          const components::ComponentContext& context)
      : samples::api::GreeterServiceBase::Component(config, context) {}

  inline void SayHello(SayHelloCall& call,
                       samples::api::GreetingRequest&& request) final;

  inline static yaml_config::Schema GetStaticConfigSchema();
};

void GreeterServiceComponent::SayHello(
    samples::api::GreeterServiceBase::SayHelloCall& call,
    samples::api::GreetingRequest&& request) {
  samples::api::GreetingResponse response;
  response.set_greeting(fmt::format("Hello, {}!", request.name()));
  call.Finish(response);
}

yaml_config::Schema GreeterServiceComponent::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<ugrpc::server::ServiceComponentBase>(R"(
type: object
description: gRPC sample greater service component
additionalProperties: false
properties: {}
)");
}

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

  inline std::string SayHello(std::string name);

  inline static yaml_config::Schema GetStaticConfigSchema();

 private:
  inline static std::unique_ptr<grpc::ClientContext> CreateClientContext(
      bool is_small_timeout);

  ugrpc::client::ClientFactory& client_factory_;
  samples::api::GreeterServiceClient client_;
};

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

std::string GreeterClient::SayHello(std::string name) {
  samples::api::GreetingRequest request;
  request.set_name(std::move(name));

  auto context = std::make_unique<grpc::ClientContext>();

  auto stream = client_.SayHello(request, std::move(context));

  samples::api::GreetingResponse response = stream.Finish();

  return std::move(*response.mutable_greeting());
}

}  // namespace functional_tests

int main(int argc, const char* const argv[]) {
  const auto component_list =
      components::MinimalServerComponentList()
          .Append<server::handlers::ServerMonitor>()
          .Append<components::TestsuiteSupport>()
          .Append<ugrpc::server::ServerComponent>()
          .Append<ugrpc::server::middlewares::log::Component>()
          .Append<ugrpc::server::middlewares::deadline_propagation::Component>()
          .Append<ugrpc::client::middlewares::log::Component>()
          .Append<ugrpc::client::ClientFactoryComponent>()
          .Append<functional_tests::GreeterClient>()
          .Append<functional_tests::GreeterServiceComponent>();

  return utils::DaemonMain(argc, argv, component_list);
}
