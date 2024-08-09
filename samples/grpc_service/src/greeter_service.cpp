#include <greeter_service.hpp>

#include <chrono>
#include <string_view>
#include <utility>

#include <fmt/format.h>

#include <userver/components/component.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/components/component_base.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/ugrpc/server/service_component_base.hpp>

#include <samples/greeter_client.usrv.pb.hpp>
#include <samples/greeter_service.usrv.pb.hpp>

namespace samples {

GreeterService::GreeterService(std::string prefix)
    : prefix_(std::move(prefix)) {}

/// [server RPC handling]
void GreeterService::SayHello(api::GreeterServiceBase::SayHelloCall& call,
                              api::GreetingRequest&& request) {
  // Authentication checking could have gone here.
  // Or even better, use a global gRPC authentication middleware.

  api::GreetingResponse response;
  response.set_greeting(fmt::format("{}, {}!", prefix_, request.name()));

  // Complete the RPC by sending the response. The service should complete
  // each request by calling `Finish` or `FinishWithError`, otherwise the
  // client will receive an Internal Error (500) response.
  call.Finish(response);
}
/// [server RPC handling]

/// [server RPC handling response_stream]
void GreeterService::SayHelloResponseStream(
    api::GreeterServiceBase::SayHelloResponseStreamCall& call,
    api::GreetingRequest&& request) {
  std::string message = fmt::format("{}, {}", prefix_, request.name());
  api::GreetingResponse response;
  constexpr auto kCountSend = 5;
  for (auto i = 0; i < kCountSend; ++i) {
    message.push_back('!');
    response.set_greeting(grpc::string(message));
    call.Write(response);
  }
  call.Finish();
}
/// [server RPC handling response_stream]

/// [server RPC handling request_stream]
void GreeterService::SayHelloRequestStream(
    api::GreeterServiceBase::SayHelloRequestStreamCall& call) {
  std::string message{};
  api::GreetingRequest request;
  while (call.Read(request)) {
    message.append(request.name());
  }
  api::GreetingResponse response;
  response.set_greeting(fmt::format("{}, {}", prefix_, message));
  call.Finish(response);
}
/// [server RPC handling request_stream]

/// [server RPC handling streams]
void GreeterService::SayHelloStreams(
    api::GreeterServiceBase::SayHelloStreamsCall& call) {
  std::string message;
  api::GreetingRequest request;
  api::GreetingResponse response;
  while (call.Read(request)) {
    message.append(request.name());
    response.set_greeting(fmt::format("{}, {}", prefix_, message));
    call.Write(response);
  }
  call.Finish();
}
/// [server RPC handling streams]

/// [component]
GreeterServiceComponent::GreeterServiceComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : ugrpc::server::ServiceComponentBase(config, context),
      service_(config["greeting-prefix"].As<std::string>()) {
  RegisterService(service_);
}
/// [component]

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

}  // namespace samples
