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

const dynamic_config::Key<ugrpc::client::ClientQos> kGreeterClientQos{
    "GREETER_CLIENT_QOS",
    dynamic_config::DefaultAsJsonString{R"({
      "methods": {
        "__default__": {
          "timeout-ms": 10000
        }
      }
    })"},
};

// A user-defined wrapper around api::GreeterServiceClient that provides
// a simplified interface.
GreeterClient::GreeterClient(api::GreeterServiceClient&& raw_client) : raw_client_(std::move(raw_client)) {}

/// [client]
std::string GreeterClient::SayHello(std::string name) const {
    api::GreetingRequest request;
    request.set_name(std::move(name));

    // Perform RPC by sending the request and receiving the response.
    api::GreetingResponse response = raw_client_.SayHello(request, MakeCallOptions());
    return std::move(*response.mutable_greeting());
}

ugrpc::client::CallOptions GreeterClient::MakeCallOptions() {
    // Deadline must be set manually for each RPC
    // Note that here in all tests the deadline equals 20 sec which works for an
    // example. However, generally speaking the deadline must be set manually for
    // each RPC
    ugrpc::client::CallOptions call_options;
    call_options.SetTimeout(std::chrono::seconds{20});
    return call_options;
}
/// [client]

/// [client_response_stream]
std::vector<std::string> GreeterClient::SayHelloResponseStream(std::string name) const {
    api::GreetingRequest request;
    request.set_name(std::move(name));
    auto stream = raw_client_.SayHelloResponseStream(request, MakeCallOptions());

    api::GreetingResponse response;
    std::vector<std::string> result;
    constexpr auto kCountSend = 5;
    for (int i = 0; i < kCountSend; i++) {
        if (!stream.Read(response)) {
            throw ugrpc::client::RpcError(stream.GetContext().GetCallName(), "Missing responses");
        }
        result.push_back(std::move(*response.mutable_greeting()));
    }

    if (stream.Read(response)) {
        throw ugrpc::client::RpcError(stream.GetContext().GetCallName(), "Extra responses");
    }
    return result;
}
/// [client_response_stream]

/// [client_request_stream]
std::string GreeterClient::SayHelloRequestStream(const std::vector<std::string_view>& names) const {
    auto stream = raw_client_.SayHelloRequestStream(MakeCallOptions());
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
    auto stream = raw_client_.SayHelloStreams(MakeCallOptions());
    std::vector<std::string> result;
    api::GreetingResponse response;
    for (const auto& name : names) {
        api::GreetingRequest request;
        request.set_name(grpc::string(name));
        stream.WriteAndCheck(request);

        if (!stream.Read(response)) {
            throw ugrpc::client::RpcError(stream.GetContext().GetCallName(), "Missing responses before WritesDone");
        }
        result.push_back(std::move(*response.mutable_greeting()));
    }
    const bool is_success = stream.WritesDone();
    LOG_DEBUG() << "Write task finish: " << is_success;
    if (stream.Read(response)) {
        throw ugrpc::client::RpcError(stream.GetContext().GetCallName(), "Extra responses after WritesDone");
    }
    return result;
}
/// [client_streams]

}  // namespace samples
