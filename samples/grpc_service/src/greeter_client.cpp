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
GreeterClient::GreeterClient(api::GreeterServiceClient&& raw_client) : raw_client_(std::move(raw_client)) {}

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
    context->set_deadline(engine::Deadline::FromDuration(std::chrono::seconds{20}));
    return context;
}
/// [client]

/// [client_response_stream]
std::vector<std::string> GreeterClient::SayHelloResponseStream(std::string name) const {
    api::GreetingRequest request;
    request.set_name(std::move(name));
    auto stream = raw_client_.SayHelloResponseStream(request, MakeClientContext());

    api::GreetingResponse response;
    std::vector<std::string> result;
    constexpr auto kCountSend = 5;
    for (int i = 0; i < kCountSend; i++) {
        if (!stream.Read(response)) {
            throw ugrpc::client::RpcError(stream.GetCallName(), "Missing responses");
        }
        result.push_back(std::move(*response.mutable_greeting()));
    }

    if (stream.Read(response)) {
        throw ugrpc::client::RpcError(stream.GetCallName(), "Extra responses");
    }
    return result;
}
/// [client_response_stream]

/// [client_request_stream]
std::string GreeterClient::SayHelloRequestStream(const std::vector<std::string_view>& names) const {
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
std::vector<std::string> GreeterClient::SayHelloStreams(const std::vector<std::string_view>& names) const {
    auto stream = raw_client_.SayHelloStreams(MakeClientContext());
    std::vector<std::string> result;
    api::GreetingResponse response;
    for (const auto& name : names) {
        api::GreetingRequest request;
        request.set_name(grpc::string(name));
        stream.WriteAndCheck(request);

        if (!stream.Read(response)) {
            throw ugrpc::client::RpcError(stream.GetCallName(), "Missing responses before WritesDone");
        }
        result.push_back(std::move(*response.mutable_greeting()));
    }
    const bool is_success = stream.WritesDone();
    LOG_DEBUG() << "Write task finish: " << is_success;
    if (stream.Read(response)) {
        throw ugrpc::client::RpcError(stream.GetCallName(), "Extra responses after WritesDone");
    }
    return result;
}
/// [client_streams]

}  // namespace samples
