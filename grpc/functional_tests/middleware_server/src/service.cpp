#include "service.hpp"

#include <fmt/format.h>

#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/ugrpc/server/service_component_base.hpp>

namespace functional_tests {

void GreeterServiceComponent::SayHello(
    samples::api::GreeterServiceBase::SayHelloCall& call,
    samples::api::GreetingRequest&& request) {
  samples::api::GreetingResponse response;
  response.set_greeting(fmt::format("Hello, {}!", request.name()));
  call.Finish(response);
}

void GreeterServiceComponent::SayHelloResponseStream(
    SayHelloResponseStreamCall& call, samples::api::GreetingRequest&& request) {
  std::string message = fmt::format("{}, {}", "Hello", request.name());
  samples::api::GreetingResponse response;
  constexpr auto kCountSend = 5;
  constexpr std::chrono::milliseconds kTimeInterval{200};
  for (auto i = 0; i < kCountSend; ++i) {
    message.push_back('!');
    response.set_greeting(grpc::string(message));
    engine::SleepFor(kTimeInterval);
    call.Write(response);
  }
  call.Finish();
}

void GreeterServiceComponent::SayHelloRequestStream(
    SayHelloRequestStreamCall& call) {
  std::string income_message;
  samples::api::GreetingRequest request;
  while (call.Read(request)) {
    income_message.append(request.name());
  }
  samples::api::GreetingResponse response;
  response.set_greeting(fmt::format("{}, {}", "Hello", income_message));
  call.Finish(response);
}

void GreeterServiceComponent::SayHelloStreams(SayHelloStreamsCall& call) {
  constexpr std::chrono::milliseconds kTimeInterval{200};
  std::string income_message;
  samples::api::GreetingRequest request;
  samples::api::GreetingResponse response;
  while (call.Read(request)) {
    income_message.append(request.name());
    response.set_greeting(fmt::format("{}, {}", "Hello", income_message));
    engine::SleepFor(kTimeInterval);
    call.Write(response);
  }
  call.Finish();
}

yaml_config::Schema GreeterServiceComponent::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<ugrpc::server::ServiceComponentBase>(R"(
type: object
description: gRPC sample greater service component
additionalProperties: false
properties: {}
)");
}

}  // namespace functional_tests
