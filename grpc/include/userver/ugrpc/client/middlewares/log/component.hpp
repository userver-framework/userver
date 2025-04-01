#pragma once

/// @file userver/ugrpc/client/middlewares/log/component.hpp
/// @brief @copybrief ugrpc::client::middlewares::log::Component

#include <userver/ugrpc/client/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

/// Client logging middleware
namespace ugrpc::client::middlewares::log {

struct Settings;

// clang-format off

/// @ingroup userver_components
///
/// @brief Component for gRPC client logging
///
/// @warning Logs are currently written with log level `debug` by default, which typically means that they are not
/// written in production. See details below.
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// msg-log-level | log level to use for request and response messages themselves | debug
/// msg-size-log-limit | max message size to log, the rest will be truncated | 512
/// trim-secrets | trim the secrets from logs as marked by the protobuf option | true (*)
///
/// @warning * Trimming secrets causes a segmentation fault for messages that contain
/// optional fields in protobuf versions prior to 3.13. You should set trim-secrets to false
/// if this is the case for you. See https://github.com/protocolbuffers/protobuf/issues/7801
///
/// ## Static configuration example:
///
/// @snippet grpc/functional_tests/basic_chaos/static_config.yaml Sample grpc client logging middleware component config
///
/// In this example, we enable logs for gRPC clients in production.

// clang-format on

class Component final : public MiddlewareFactoryComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of ugrpc::client::middlewares::log::Component.
    static constexpr std::string_view kName = "grpc-client-logging";

    Component(const components::ComponentConfig& config, const components::ComponentContext& context);

    ~Component() override;

    static yaml_config::Schema GetStaticConfigSchema();

    yaml_config::Schema GetMiddlewareConfigSchema() const override;

    std::shared_ptr<MiddlewareBase>
    CreateMiddleware(const ugrpc::client::ClientInfo&, const yaml_config::YamlConfig& middleware_config) const override;
};

}  // namespace ugrpc::client::middlewares::log

template <>
inline constexpr bool components::kHasValidate<ugrpc::client::middlewares::log::Component> = true;

template <>
inline constexpr auto components::kConfigFileMode<ugrpc::client::middlewares::log::Component> =
    ConfigFileMode::kNotRequired;

USERVER_NAMESPACE_END
