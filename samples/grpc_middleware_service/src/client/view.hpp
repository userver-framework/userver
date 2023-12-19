#pragma once

#include <userver/utest/using_namespace_userver.hpp>

#include <userver/components/component.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <samples/greeter_client.usrv.pb.hpp>
#include <userver/ugrpc/client/client_factory_component.hpp>

namespace samples::grpc::auth {

class GreeterClient final : public components::LoggableComponentBase {
 public:
  static constexpr std::string_view kName = "greeter-client";

  GreeterClient(const components::ComponentConfig& config,
                const components::ComponentContext& context);

  std::string SayHello(std::string name);

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  ugrpc::client::ClientFactory& client_factory_;
  api::GreeterServiceClient client_;
};

}  // namespace samples::grpc::auth
