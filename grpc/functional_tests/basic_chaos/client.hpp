#pragma once

#include <userver/utest/using_namespace_userver.hpp>

#include <userver/components/component.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/engine/async.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/ugrpc/client/client_factory_component.hpp>

#include <samples/greeter_client.usrv.pb.hpp>

namespace samples {

class GreeterClient final : public components::LoggableComponentBase {
 public:
  static constexpr std::string_view kName = "greeter-client";

  GreeterClient(const components::ComponentConfig& config,
                const components::ComponentContext& context)
      : LoggableComponentBase(config, context),
        client_factory_(
            context.FindComponent<ugrpc::client::ClientFactoryComponent>()
                .GetFactory()),
        client_(client_factory_.MakeClient<api::GreeterServiceClient>(
            "greeter", config["endpoint"].As<std::string>())) {}

  inline std::string SayHello(std::string name, bool is_small_timeout);

  inline std::string SayHelloResponseStream(std::string name,
                                            bool is_small_timeout);

  inline std::string SayHelloRequestStream(
      const std::vector<std::string>& names, bool is_small_timeout);

  inline std::string SayHelloStreams(const std::vector<std::string>& names,
                                     bool is_small_timeout);

  inline std::string SayHelloIndependentStreams(
      const std::vector<std::string>& names, bool is_small_timeout);

  inline static yaml_config::Schema GetStaticConfigSchema();

 private:
  inline static std::unique_ptr<grpc::ClientContext> CreateClientContext(
      bool is_small_timeout);

  ugrpc::client::ClientFactory& client_factory_;
  api::GreeterServiceClient client_;
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

std::unique_ptr<grpc::ClientContext> GreeterClient::CreateClientContext(
    bool is_small_timeout) {
  auto context = std::make_unique<grpc::ClientContext>();
  context->set_deadline(engine::Deadline::FromDuration(
      std::chrono::seconds{is_small_timeout ? 1 : 20}));
  context->set_wait_for_ready(true);
  return context;
}

std::string GreeterClient::SayHello(std::string name, bool is_small_timeout) {
  api::GreetingRequest request;
  request.set_name(std::move(name));

  auto stream =
      client_.SayHello(request, CreateClientContext(is_small_timeout));

  api::GreetingResponse response = stream.Finish();

  return std::move(*response.mutable_greeting());
}

std::string GreeterClient::SayHelloResponseStream(std::string name,
                                                  bool is_small_timeout) {
  std::string result{};
  api::GreetingRequest request;
  request.set_name(std::move(name));

  auto stream = client_.SayHelloResponseStream(
      request, CreateClientContext(is_small_timeout));
  api::GreetingResponse response;
  while (stream.Read(response)) {
    result.append(response.greeting());
    result.append("\n");
  }
  return result;
}

std::string GreeterClient::SayHelloRequestStream(
    const std::vector<std::string>& names, bool is_small_timeout) {
  std::string result{};
  auto stream =
      client_.SayHelloRequestStream(CreateClientContext(is_small_timeout));
  for (const auto& name : names) {
    api::GreetingRequest request;
    request.set_name(grpc::string(name));
    if (!stream.Write(request)) {
      return "Error write";
    }
  }
  auto response = stream.Finish();
  return std::move(*response.mutable_greeting());
}

std::string GreeterClient::SayHelloStreams(
    const std::vector<std::string>& names, bool is_small_timeout) {
  std::string result{};
  auto stream = client_.SayHelloStreams(CreateClientContext(is_small_timeout));
  for (const auto& name : names) {
    api::GreetingRequest request;
    request.set_name(grpc::string(name));
    stream.WriteAndCheck(request);
    api::GreetingResponse response;
    if (stream.Read(response)) {
      result.append(std::move(*response.mutable_greeting()));
      result.append("\n");
    }
  }
  return result;
}

std::string GreeterClient::SayHelloIndependentStreams(
    const std::vector<std::string>& names, bool is_small_timeout) {
  std::string result{};
  auto stream =
      client_.SayHelloIndependentStreams(CreateClientContext(is_small_timeout));
  auto write_task = engine::AsyncNoSpan([&stream, &names] {
    for (const auto& name : names) {
      api::GreetingRequest request;
      request.set_name(grpc::string(name));
      if (!stream.Write(request)) {
        throw std::runtime_error("Write error");
      }
    }
    const bool is_success = stream.WritesDone();
    LOG_DEBUG() << "Write task finish: " << is_success;
  });

  auto read_task = engine::AsyncNoSpan([&stream, &result] {
    api::GreetingResponse response;
    while (stream.Read(response)) {
      result.append(std::move(*response.mutable_greeting()));
      result.append("\n");
    }
  });
  write_task.Get();
  read_task.Get();
  return result;
}

}  // namespace samples
