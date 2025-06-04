#pragma once

/// @file userver/ugrpc/server/middlewares/access_log/component.hpp
/// @brief @copybrief ugrpc::server::middlewares::access_log::Component

#include <userver/logging/component.hpp>

#include <userver/ugrpc/server/middlewares/base.hpp>

USERVER_NAMESPACE_BEGIN

/// Server access_log middleware
/// @see @ref scripts/docs/en/userver/grpc/server_middlewares.md
/// @see @ref ugrpc::server::middlewares::access_log::Component
namespace ugrpc::server::middlewares::access_log {

/// @ingroup userver_components userver_base_classes
///
/// @brief gRPC server access log middleware component. Writes one TSKV log line per handled RPC in a static format.
/// This log is intended to be collected, parsed and stored for audit or extended statistics purposes.
///
/// This middleware is disabled and unregistered by default.
///
/// ## Static options:
/// The component name for static config is `"grpc-server-access-log"`.
///
/// Name | Description | Default value
/// ---- | ----------- | -------------
/// `access-tskv-logger` | logger name for `access-tskv.log`, must have `format: raw` | -
///
/// ## Static configuration example:
///
/// @snippet grpc/functional_tests/basic_chaos/static_config.yaml grpc server access log
///
/// @see @ref scripts/docs/en/userver/grpc/server_middlewares.md
class Component final : public MiddlewareFactoryComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of ugrpc::server::middlewares::access_log::Component
    static constexpr std::string_view kName = "grpc-server-access-log";

    Component(const components::ComponentConfig& config, const components::ComponentContext& context);

    std::shared_ptr<const MiddlewareBase> CreateMiddleware(
        const ugrpc::server::ServiceInfo&,
        const yaml_config::YamlConfig& middleware_config
    ) const override;

    yaml_config::Schema GetMiddlewareConfigSchema() const override;

    static yaml_config::Schema GetStaticConfigSchema();

private:
    components::Logging& logging_component_;
};

}  // namespace ugrpc::server::middlewares::access_log

template <>
inline constexpr bool components::kHasValidate<ugrpc::server::middlewares::access_log::Component> = true;

template <>
inline constexpr auto components::kConfigFileMode<ugrpc::server::middlewares::access_log::Component> =
    ConfigFileMode::kNotRequired;

USERVER_NAMESPACE_END
