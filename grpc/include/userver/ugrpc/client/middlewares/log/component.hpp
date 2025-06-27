#pragma once

/// @file userver/ugrpc/client/middlewares/log/component.hpp
/// @brief @copybrief ugrpc::client::middlewares::log::Component

#include <userver/ugrpc/client/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

/// Client logging middleware
/// @see @ref scripts/docs/en/userver/grpc/client_middlewares.md
/// @see @ref ugrpc::client::middlewares::log::Component
namespace ugrpc::client::middlewares::log {

struct Settings;

/// @ingroup userver_components
///
/// @brief Component for gRPC client logging
///
/// `google::protobuf::Message` fields with the option `[debug_redact = true]` are logged as `[REDACTED]` string
/// to avoid print secrets in logs. `debug_redact` is available in protobuf version >= 22.
///
/// @warning Logs are currently written with log level `debug` by default, which typically means that they are not
/// written in production. See details below.
///
/// ## Static options:
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// log-level | log level threshold | debug
/// msg-log-level | logging level to use for request and response messages themselves | debug
/// msg-size-log-limit | max message size to log, the rest will be truncated | 512
///
/// ## Static configuration example:
///
/// @snippet grpc/functional_tests/basic_chaos/static_config.yaml Sample grpc client logging middleware component config
///
/// In this example, we enable logs for gRPC clients in production.
///
/// @see @ref scripts/docs/en/userver/grpc/client_middlewares.md

class Component final : public MiddlewareFactoryComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of ugrpc::client::middlewares::log::Component.
    static constexpr std::string_view kName = "grpc-client-logging";

    Component(const components::ComponentConfig& config, const components::ComponentContext& context);

    ~Component() override;

    static yaml_config::Schema GetStaticConfigSchema();

    yaml_config::Schema GetMiddlewareConfigSchema() const override;

    std::shared_ptr<const MiddlewareBase>
    CreateMiddleware(const ugrpc::client::ClientInfo&, const yaml_config::YamlConfig& middleware_config) const override;
};

}  // namespace ugrpc::client::middlewares::log

template <>
inline constexpr bool components::kHasValidate<ugrpc::client::middlewares::log::Component> = true;

template <>
inline constexpr auto components::kConfigFileMode<ugrpc::client::middlewares::log::Component> =
    ConfigFileMode::kNotRequired;

USERVER_NAMESPACE_END
