#pragma once

/// @file userver/ugrpc/server/middlewares/log/component.hpp
/// @brief @copybrief ugrpc::server::middlewares::log::Component

#include <userver/ugrpc/server/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

/// Server logging middleware
/// @see @ref scripts/docs/en/userver/grpc/server_middlewares.md
/// @see @ref ugrpc::server::middlewares::log::Component
namespace ugrpc::server::middlewares::log {

/// @ingroup userver_components userver_base_classes
///
/// @brief Component for gRPC server logging
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
/// local-log-level | local log level for the span with server logs | debug
///
/// @warning * Trimming secrets causes a segmentation fault for messages that contain
/// optional fields in protobuf versions prior to 3.13. You should set trim-secrets to false
/// if this is the case for you. See https://github.com/protocolbuffers/protobuf/issues/7801
///
/// ## Static configuration example:
///
/// @snippet grpc/functional_tests/basic_chaos/static_config.yaml Sample grpc server logging middleware component config
///
/// In this example, we enable logs for gRPC clients in production.
///
/// @see @ref scripts/docs/en/userver/grpc/server_middlewares.md
class Component final : public MiddlewareFactoryComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of ugrpc::server::middlewares::log::Component
    static constexpr std::string_view kName = "grpc-server-logging";

    Component(const components::ComponentConfig& config, const components::ComponentContext& context);

    static yaml_config::Schema GetStaticConfigSchema();

    yaml_config::Schema GetMiddlewareConfigSchema() const override;

    std::shared_ptr<const MiddlewareBase> CreateMiddleware(
        const ugrpc::server::ServiceInfo&,
        const yaml_config::YamlConfig& middleware_config
    ) const override;
};

}  // namespace ugrpc::server::middlewares::log

template <>
inline constexpr bool components::kHasValidate<ugrpc::server::middlewares::log::Component> = true;

template <>
inline constexpr auto components::kConfigFileMode<ugrpc::server::middlewares::log::Component> =
    ConfigFileMode::kNotRequired;

USERVER_NAMESPACE_END
