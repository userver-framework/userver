#include <userver/utest/using_namespace_userver.hpp>

#include <userver/components/minimal_server_component_list.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/utils/daemon_run.hpp>
#include <userver/ugrpc/server/server_component.hpp>
#include <userver/ugrpc/server/service_component_base.hpp>
#include <userver/yaml_config/merge_schemas.hpp>
#include <userver/storages/clickhouse/component.hpp>

#include <greeter_client.usrv.pb.hpp>
#include <greeter_service.usrv.pb.hpp>

class Hello final : public server::handlers::HttpHandlerBase {
 public:
  // `kName` is used as the component name in static config
  static constexpr std::string_view kName = "handler-hello-sample";

  // Component is valid after construction and is able to accept requests
  using HttpHandlerBase::HttpHandlerBase;

  std::string HandleRequestThrow(
      const server::http::HttpRequest&,
      server::request::RequestContext&) const override {
    return "Hello world!\n";
  }
};

class GreeterService final : public api::GreeterServiceBase {
 public:
  explicit GreeterService(std::string greeting_prefix)
      : prefix_(std::move(greeting_prefix)) {}

  void SayHello(SayHelloCall& call, api::GreetingRequest&& request) override {
    // Authentication checking could have gone here. For this example, we trust
    // the world.

    api::GreetingResponse response;
    response.set_greeting(fmt::format("{}, {}!", prefix_, request.name()));

    // Complete the RPC by sending the response. The service should complete
    // each request by calling `Finish` or `FinishWithError`, otherwise the
    // client will receive an Internal Error (500) response.
    call.Finish(response);
  }

 private:
  std::string prefix_;
};

class GreeterServiceComponent final
    : public ugrpc::server::ServiceComponentBase {
 public:
  static constexpr std::string_view kName = "greeter-service";

  GreeterServiceComponent(const components::ComponentConfig& config,
                          const components::ComponentContext& context)
      : ugrpc::server::ServiceComponentBase(config, context),
        // Configuration and dependency injection for the gRPC service
        // implementation happens here.
        service_(config["greeting-prefix"].As<std::string>()) {
    // The ServiceComponentBase-derived component must provide a service
    // interface implementation here.
    RegisterService(service_);
  }

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  GreeterService service_;
};

yaml_config::Schema GreeterServiceComponent::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<ugrpc::server::ServiceComponentBase>(R"(
type: object
description: gRPC sample greater service component
additionalProperties: false
properties:
    greeting-prefix:
        type: string
        description: greeting prefix
)");
}

int main(int argc, char* argv[]) {
  const auto component_list =
      components::MinimalServerComponentList()
          .Append<Hello>()
          .Append<GreeterServiceComponent>()
          .Append<userver::components::ClickHouse>("clickhouse-database");
  return 0;
}
