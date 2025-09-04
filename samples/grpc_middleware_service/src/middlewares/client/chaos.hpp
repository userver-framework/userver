#pragma once

#include <userver/utest/using_namespace_userver.hpp>

#include <userver/ugrpc/client/middlewares/base.hpp>

namespace samples::grpc::auth::client {

class ChaosMiddleware final : public ugrpc::client::MiddlewareBase {
public:
    explicit ChaosMiddleware(bool feature_enabled_);

    void PreStartCall(ugrpc::client::MiddlewareCallContext& context) const override;

private:
    bool feature_enabled_;
};

/// [gRPC middleware sample]
class ChaosComponent final : public ugrpc::client::MiddlewareFactoryComponentBase {
public:
    static constexpr std::string_view kName = "grpc-chaos-client";

    ChaosComponent(const components::ComponentConfig& config, const components::ComponentContext& context);

    // Needed to pass static config options to the middleware.
    yaml_config::Schema GetMiddlewareConfigSchema() const override;

    static yaml_config::Schema GetStaticConfigSchema();

    std::shared_ptr<const MiddlewareBase>
    CreateMiddleware(const ugrpc::client::ClientInfo&, const yaml_config::YamlConfig& middleware_config) const override;
};
/// [gRPC middleware sample]

}  // namespace samples::grpc::auth::client
