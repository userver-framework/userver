#include <userver/utest/using_namespace_userver.hpp>

#include <chrono>
#include <string_view>
#include <utility>

#include <fmt/format.h>

#include <userver/components/component.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/components/minimal_server_component_list.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/utils/daemon_run.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

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
        prefix_(config["greeting-prefix"].As<std::string>()) {}

  void SayHello(SayHelloCall& call, api::GreetingRequest&& request) final;

  void SayHelloResponseStream(SayHelloResponseStreamCall& call,
                              api::GreetingRequest&& request) final;

  void SayHelloRequestStream(SayHelloRequestStreamCall& call) final;

  void SayHelloStreams(SayHelloStreamsCall& call) final;

  void SayHelloIndependentStreams(SayHelloIndependentStreamsCall& call) final;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  const std::string prefix_;
};

void GreeterServiceComponent::SayHello(
    api::GreeterServiceBase::SayHelloCall& call,
    api::GreetingRequest&& request) {
  api::GreetingResponse response;
  response.set_greeting(fmt::format("{}, {}!", prefix_, request.name()));

  call.Finish(response);
}

void GreeterServiceComponent::SayHelloResponseStream(
    SayHelloResponseStreamCall& call, api::GreetingRequest&& request) {
  std::string message = fmt::format("{}, {}", prefix_, request.name());
  api::GreetingResponse response;
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
  api::GreetingRequest request;
  while (call.Read(request)) {
    income_message.append(request.name());
  }
  api::GreetingResponse response;
  response.set_greeting(fmt::format("{}, {}", prefix_, income_message));
  call.Finish(response);
}

void GreeterServiceComponent::SayHelloStreams(SayHelloStreamsCall& call) {
  constexpr std::chrono::milliseconds kTimeInterval{200};
  std::string income_message;
  api::GreetingRequest request;
  api::GreetingResponse response;
  while (call.Read(request)) {
    income_message.append(request.name());
    response.set_greeting(fmt::format("{}, {}", prefix_, income_message));
    engine::SleepFor(kTimeInterval);
    call.Write(response);
  }
  call.Finish();
}

void GreeterServiceComponent::SayHelloIndependentStreams(
    SayHelloIndependentStreamsCall& call) {
  constexpr std::chrono::milliseconds kTimeIntervalRead{200};
  constexpr std::chrono::milliseconds kTimeIntervalWrite{300};
  constexpr auto kCountResponseMessage = 10;

  std::string final_string{};
  auto read_task =
      engine::AsyncNoSpan([&final_string, &call, &kTimeIntervalRead] {
        api::GreetingRequest request;
        while (call.Read(request)) {
          final_string.append(request.name());
          engine::SleepFor(kTimeIntervalRead);
        }
      });

  auto write_task =
      engine::AsyncNoSpan([&call, prefix = prefix_, &kTimeIntervalWrite] {
        api::GreetingResponse response;
        std::array<std::string, kCountResponseMessage> kNames = {
            "Python", "C++",       "linux", "userver", "grpc",
            "kernel", "developer", "core",  "anonim",  "user"};
        for (auto i = 0; i < kCountResponseMessage; ++i) {
          response.set_greeting(fmt::format("{}, {}", prefix, kNames[i]));
          call.Write(response);
          engine::SleepFor(kTimeIntervalWrite);
        }
      });

  read_task.Get();
  write_task.Get();

  api::GreetingResponse response;
  response.set_greeting(grpc::string(final_string));
  call.WriteAndFinish(response);
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

int main(int argc, char* argv[]) {
  const auto component_list = components::MinimalServerComponentList()
                                  .Append<ugrpc::server::ServerComponent>()
                                  .Append<samples::GreeterServiceComponent>();
  return utils::DaemonMain(argc, argv, component_list);
}
