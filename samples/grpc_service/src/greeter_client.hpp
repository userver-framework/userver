#pragma once

#include <string_view>

#include <userver/utest/using_namespace_userver.hpp>

/// [includes]
#include <userver/components/component_base.hpp>
#include <userver/components/component_fwd.hpp>

#include <userver/ugrpc/client/fwd.hpp>
#include <userver/ugrpc/client/simple_client_component.hpp>

#include <samples/greeter_client.usrv.pb.hpp>
/// [includes]

namespace samples {

/// [client]
// A user-defined wrapper around api::GreeterServiceClient that handles
// the metadata and deadline bureaucracy and provides a simplified interface.
//
// Alternatively, you can use ugrpc::client::SimpleClientComponent directly.
//
// Note that we have both service and client to that service in the same
// microservice. Ignore that, it's just for the sake of example.
class GreeterClient final {
public:
    explicit GreeterClient(api::GreeterServiceClient&& raw_client);

    std::string SayHello(std::string name) const;

    std::vector<std::string> SayHelloResponseStream(std::string name) const;

    std::string SayHelloRequestStream(const std::vector<std::string_view>& names) const;

    std::vector<std::string> SayHelloStreams(const std::vector<std::string_view>& names) const;

private:
    static std::unique_ptr<grpc::ClientContext> MakeClientContext();

    api::GreeterServiceClient raw_client_;
};
/// [client]

using Client = GreeterClient;

/// [component]
class GreeterClientComponent final : public ugrpc::client::SimpleClientComponent<Client> {
public:
    static constexpr std::string_view kName = "greeter-client";

    using Base = ugrpc::client::SimpleClientComponent<Client>;

    GreeterClientComponent(const ::components::ComponentConfig& config, const ::components::ComponentContext& context)
        : Base(config, context) {}

    using Base::GetClient;
};
/// [component]

}  // namespace samples
