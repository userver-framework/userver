#pragma once

#include <userver/utest/using_namespace_userver.hpp>

#include <userver/ugrpc/server/server_component.hpp>
#include <userver/ugrpc/server/service_component_base.hpp>

#include <samples/greeter_service.usrv.pb.hpp>

namespace samples::grpc::auth {

class GreeterServiceComponent final
    : public api::GreeterServiceBase::Component {
 public:
  static constexpr std::string_view kName = "greeter-service";

  GreeterServiceComponent(const components::ComponentConfig& config,
                          const components::ComponentContext& context);

  void SayHello(SayHelloCall& call, api::GreetingRequest&& request) override;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  const std::string prefix_;
};

}  // namespace samples::grpc::auth
