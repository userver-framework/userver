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

  SayHelloResult SayHello(CallContext& context,
                          samples::api::GreetingRequest&& request) final;

  SayHelloResponseStreamResult SayHelloResponseStream(
      CallContext& context, samples::api::GreetingRequest&& request,
      SayHelloResponseStreamWriter& writer) final;

  SayHelloRequestStreamResult SayHelloRequestStream(
      CallContext& context, SayHelloRequestStreamReader& reader) final;

  SayHelloStreamsResult SayHelloStreams(
      CallContext& context, SayHelloStreamsReaderWriter& stream) final;

  static yaml_config::Schema GetStaticConfigSchema();
};

}  // namespace functional_tests
