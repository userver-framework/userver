#pragma once
#include <userver/utest/using_namespace_userver.hpp>

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

  void SayHello(SayHelloCall& call,
                samples::api::GreetingRequest&& request) final;

  void SayHelloResponseStream(SayHelloResponseStreamCall& call,
                              samples::api::GreetingRequest&& request) final;

  void SayHelloRequestStream(SayHelloRequestStreamCall& call) final;

  void SayHelloStreams(SayHelloStreamsCall& call) final;

  static yaml_config::Schema GetStaticConfigSchema();
};

}  // namespace functional_tests
