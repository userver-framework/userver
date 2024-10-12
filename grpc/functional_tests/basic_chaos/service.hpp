#pragma once
#include <userver/utest/using_namespace_userver.hpp>

#include <array>
#include <chrono>
#include <string_view>
#include <utility>

#include <fmt/format.h>

#include <userver/components/component.hpp>
#include <userver/components/component_base.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/testsuite/testpoint.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

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
        prefix_(config["greeting-prefix"].As<std::string>()) {}

  inline SayHelloResult SayHello(CallContext& context,
                                 api::GreetingRequest&& request) final;

  inline SayHelloResponseStreamResult SayHelloResponseStream(
      CallContext& context, api::GreetingRequest&& request,
      SayHelloResponseStreamWriter& writer) final;

  inline SayHelloRequestStreamResult SayHelloRequestStream(
      CallContext& context, SayHelloRequestStreamReader& reader) final;

  inline SayHelloStreamsResult SayHelloStreams(
      CallContext& context, SayHelloStreamsReaderWriter& stream) final;

  inline SayHelloIndependentStreamsResult SayHelloIndependentStreams(
      CallContext& context,
      SayHelloIndependentStreamsReaderWriter& stream) final;

  inline static yaml_config::Schema GetStaticConfigSchema();

 private:
  const std::string prefix_;
};

GreeterServiceComponent::SayHelloResult GreeterServiceComponent::SayHello(
    CallContext& /*context*/, api::GreetingRequest&& request) {
  api::GreetingResponse response;

  if (request.name() == "test_payload_cancellation") {
    engine::InterruptibleSleepFor(std::chrono::seconds(20));
    if (engine::current_task::IsCancelRequested()) {
      engine::TaskCancellationBlocker block_cancel;
      TESTPOINT("testpoint_cancel", {});
    }
  }

  response.set_greeting(fmt::format("{}, {}!", prefix_, request.name()));
  return response;
}

GreeterServiceComponent::SayHelloResponseStreamResult
GreeterServiceComponent::SayHelloResponseStream(
    CallContext& /*context*/, api::GreetingRequest&& request,
    SayHelloResponseStreamWriter& writer) {
  std::string message = fmt::format("{}, {}", prefix_, request.name());
  api::GreetingResponse response;
  constexpr auto kCountSend = 5;
  constexpr std::chrono::milliseconds kTimeInterval{200};
  for (auto i = 0; i < kCountSend; ++i) {
    message.push_back('!');
    response.set_greeting(grpc::string(message));
    engine::SleepFor(kTimeInterval);
    writer.Write(response);
  }
  return grpc::Status::OK;
}

GreeterServiceComponent::SayHelloRequestStreamResult
GreeterServiceComponent::SayHelloRequestStream(
    CallContext& /*context*/, SayHelloRequestStreamReader& reader) {
  std::string income_message;
  api::GreetingRequest request;
  while (reader.Read(request)) {
    income_message.append(request.name());
  }
  api::GreetingResponse response;
  response.set_greeting(fmt::format("{}, {}", prefix_, income_message));
  return response;
}

GreeterServiceComponent::SayHelloStreamsResult
GreeterServiceComponent::SayHelloStreams(CallContext& /*context*/,
                                         SayHelloStreamsReaderWriter& stream) {
  constexpr std::chrono::milliseconds kTimeInterval{200};
  std::string income_message;
  api::GreetingRequest request;
  api::GreetingResponse response;
  while (stream.Read(request)) {
    income_message.append(request.name());
    response.set_greeting(fmt::format("{}, {}", prefix_, income_message));
    engine::SleepFor(kTimeInterval);
    stream.Write(response);
  }
  return grpc::Status::OK;
}

GreeterServiceComponent::SayHelloIndependentStreamsResult
GreeterServiceComponent::SayHelloIndependentStreams(
    CallContext& /*context*/, SayHelloIndependentStreamsReaderWriter& stream) {
  constexpr std::chrono::milliseconds kTimeIntervalRead{200};
  constexpr std::chrono::milliseconds kTimeIntervalWrite{300};

  std::string final_string{};
  auto read_task =
      engine::AsyncNoSpan([&final_string, &stream, &kTimeIntervalRead] {
        api::GreetingRequest request;
        while (stream.Read(request)) {
          final_string.append(request.name());
          engine::SleepFor(kTimeIntervalRead);
        }
      });

  auto write_task = engine::AsyncNoSpan([&stream, prefix = prefix_,
                                         &kTimeIntervalWrite] {
    api::GreetingResponse response;
    std::array kNames = {"Python", "C++",       "linux", "userver",   "grpc",
                         "kernel", "developer", "core",  "anonymous", "user"};
    for (const auto& name : kNames) {
      response.set_greeting(fmt::format("{}, {}", prefix, name));
      stream.Write(response);
      engine::SleepFor(kTimeIntervalWrite);
    }
  });

  read_task.Get();
  write_task.Get();

  api::GreetingResponse response;
  response.set_greeting(grpc::string(final_string));
  stream.Write(response);
  return grpc::Status::OK;
}

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
