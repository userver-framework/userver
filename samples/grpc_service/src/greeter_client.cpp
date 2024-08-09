#include <greeter_client.hpp>

#include <chrono>
#include <memory>
#include <string>
#include <utility>

#include <userver/components/component.hpp>
#include <userver/engine/deadline.hpp>
#include <userver/ugrpc/client/client_factory_component.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

namespace samples {

// A user-defined wrapper around api::GreeterServiceClient that provides
// a simplified interface.
GreeterClient::GreeterClient(api::GreeterServiceClient&& raw_client)
    : raw_client_(std::move(raw_client)) {}

/// [client]
std::string GreeterClient::SayHello(std::string name) const {
  api::GreetingRequest request;
  request.set_name(std::move(name));

  // Initiate the RPC. No actual actions have been taken thus far besides
  // preparing to send the request.
  auto stream = raw_client_.SayHello(request, MakeClientContext());

  // Complete the unary RPC by sending the request and receiving the response.
  // The client should call `Finish` (in case of single response) or `Read`
  // until `false` (in case of response stream), otherwise the RPC will be
  // cancelled.
  api::GreetingResponse response = stream.Finish();
  return std::move(*response.mutable_greeting());
}

std::unique_ptr<grpc::ClientContext> GreeterClient::MakeClientContext() {
  // Deadline must be set manually for each RPC
  // Note that here in all tests the deadline equals 20 sec which works for an
  // example. However, generally speaking the deadline must be set manually for
  // each RPC
  auto context = std::make_unique<grpc::ClientContext>();
  context->set_deadline(
      engine::Deadline::FromDuration(std::chrono::seconds{20}));
  return context;
}
/// [client]

/// [client_response_stream]
std::vector<std::string> GreeterClient::SayHelloResponseStream(
    std::string name) const {
  api::GreetingRequest request;
  request.set_name(std::move(name));
  auto stream =
      raw_client_.SayHelloResponseStream(request, MakeClientContext());

  api::GreetingResponse response;
  std::vector<std::string> result;
  while (stream.Read(response)) {
    result.push_back(std::move(*response.mutable_greeting()));
  }
  return result;
}
/// [client_response_stream]

/// [client_request_stream]
std::string GreeterClient::SayHelloRequestStream(
    const std::vector<std::string_view>& names) const {
  auto stream = raw_client_.SayHelloRequestStream(MakeClientContext());
  for (const auto& name : names) {
    api::GreetingRequest request;
    request.set_name(grpc::string(name));
    stream.WriteAndCheck(request);
  }
  auto response = stream.Finish();
  return std::move(*response.mutable_greeting());
}
/// [client_request_stream]

/// [client_streams]
std::vector<std::string> GreeterClient::SayHelloStreams(
    const std::vector<std::string_view>& names) const {
  auto stream = raw_client_.SayHelloStreams(MakeClientContext());
  std::vector<std::string> result;
  api::GreetingResponse response;
  for (const auto& name : names) {
    api::GreetingRequest request;
    request.set_name(grpc::string(name));
    stream.WriteAndCheck(request);

    if (!stream.Read(response)) {
      throw ugrpc::client::RpcError(stream.GetCallName(),
                                    "Missing responses before WritesDone");
    }
    result.push_back(std::move(*response.mutable_greeting()));
  }
  const bool is_success = stream.WritesDone();
  LOG_DEBUG() << "Write task finish: " << is_success;
  if (stream.Read(response)) {
    throw ugrpc::client::RpcError(stream.GetCallName(),
                                  "Extra responses after WritesDone");
  }
  return result;
}
/// [client_streams]

/// [component]
GreeterClientComponent::GreeterClientComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : ComponentBase(config, context),
      // ClientFactory is used to create gRPC clients
      client_factory_(
          context.FindComponent<ugrpc::client::ClientFactoryComponent>()
              .GetFactory()),
      // The client needs a fixed endpoint
      client_(client_factory_.MakeClient<api::GreeterServiceClient>(
          "greeter", config["endpoint"].As<std::string>())) {}
/// [component]

const GreeterClient& GreeterClientComponent::GetClient() const {
  return client_;
}

yaml_config::Schema GreeterClientComponent::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<components::ComponentBase>(R"(
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

}  // namespace samples
