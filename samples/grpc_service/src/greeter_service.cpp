#include <greeter_service.hpp>

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

GreeterService::GreeterService(std::string prefix) : prefix_(std::move(prefix)) {}

/// [server RPC handling]
GreeterService::SayHelloResult GreeterService::SayHello(CallContext& /*context*/, api::GreetingRequest&& request) {
    // Authentication checking could have gone here.
    // Or even better, use a global gRPC authentication middleware.

    api::GreetingResponse response;
    response.set_greeting(fmt::format("{}, {}!", prefix_, request.name()));

    // Complete the RPC by returning the response.
    return response;
}
/// [server RPC handling]

/// [server RPC handling response_stream]
GreeterService::SayHelloResponseStreamResult GreeterService::SayHelloResponseStream(
    CallContext& /*context*/,
    api::GreetingRequest&& request,
    SayHelloResponseStreamWriter& writer
) {
    std::string message = fmt::format("{}, {}", prefix_, request.name());
    api::GreetingResponse response;
    constexpr auto kCountSend = 5;
    for (auto i = 0; i < kCountSend; ++i) {
        message.push_back('!');
        response.set_greeting(grpc::string(message));
        writer.Write(response);
    }
    return grpc::Status::OK;
}
/// [server RPC handling response_stream]

/// [server RPC handling request_stream]
GreeterService::SayHelloRequestStreamResult
GreeterService::SayHelloRequestStream(CallContext& /*context*/, SayHelloRequestStreamReader& reader) {
    std::string message{};
    api::GreetingRequest request;
    while (reader.Read(request)) {
        message.append(request.name());
    }
    api::GreetingResponse response;
    response.set_greeting(fmt::format("{}, {}", prefix_, message));
    return response;
}
/// [server RPC handling request_stream]

/// [server RPC handling streams]
GreeterService::SayHelloStreamsResult
GreeterService::SayHelloStreams(CallContext& /*context*/, SayHelloStreamsReaderWriter& stream) {
    std::string message;
    api::GreetingRequest request;
    api::GreetingResponse response;
    while (stream.Read(request)) {
        message.append(request.name());
        response.set_greeting(fmt::format("{}, {}", prefix_, message));
        stream.Write(response);
    }
    return grpc::Status::OK;
}
/// [server RPC handling streams]

/// [component]
GreeterServiceComponent::GreeterServiceComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
)
    : ugrpc::server::ServiceComponentBase(config, context), service_(config["greeting-prefix"].As<std::string>()) {
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
