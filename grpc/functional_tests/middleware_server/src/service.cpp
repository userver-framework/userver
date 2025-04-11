#include "service.hpp"

#include <fmt/format.h>

#include <userver/engine/async.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/ugrpc/server/service_component_base.hpp>

namespace functional_tests {

GreeterServiceComponent::SayHelloResult
GreeterServiceComponent::SayHello(CallContext& /*context*/, samples::api::GreetingRequest&& request) {
    samples::api::GreetingResponse response;
    response.set_greeting(fmt::format("Hello, {}!", request.name()));
    return response;
}

GreeterServiceComponent::SayHelloResponseStreamResult GreeterServiceComponent::SayHelloResponseStream(
    CallContext& /*context*/,
    samples::api::GreetingRequest&& request,
    SayHelloResponseStreamWriter& writer
) {
    std::string message = fmt::format("{}, {}", "Hello", request.name());
    samples::api::GreetingResponse response;
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
GreeterServiceComponent::SayHelloRequestStream(CallContext& /*context*/, SayHelloRequestStreamReader& reader) {
    std::string income_message;
    samples::api::GreetingRequest request;
    while (reader.Read(request)) {
        income_message.append(request.name());
    }
    samples::api::GreetingResponse response;
    response.set_greeting(fmt::format("{}, {}", "Hello", income_message));
    return response;
}

GreeterServiceComponent::SayHelloStreamsResult
GreeterServiceComponent::SayHelloStreams(CallContext& /*context*/, SayHelloStreamsReaderWriter& stream) {
    constexpr std::chrono::milliseconds kTimeInterval{200};
    std::string income_message;
    samples::api::GreetingRequest request;
    samples::api::GreetingResponse response;
    while (stream.Read(request)) {
        income_message.append(request.name());
        response.set_greeting(fmt::format("{}, {}", "Hello", income_message));
        engine::SleepFor(kTimeInterval);
        stream.Write(response);
    }
    return grpc::Status::OK;
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
