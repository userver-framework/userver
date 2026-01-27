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
/// `google::protobuf::Message` fields with the option `[debug_redact = true]` are logged as `[REDACTED]` string
/// to avoid print secrets in logs. `debug_redact` is available in protobuf version >= 22.
///
/// @warning Logs are currently written with log level `debug` by default, which typically means that they are not
/// written in production. See details below.
///
/// ## Static options of ugrpc::server::middlewares::log::Component :
/// @include{doc} scripts/docs/en/components_schema/grpc/src/ugrpc/server/middlewares/log/component.md
///
/// Options inherited from @ref middlewares::MiddlewareFactoryComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/middlewares/factory_component_base.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
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
inline constexpr auto
    components::kConfigFileMode<ugrpc::server::middlewares::log::Component> = ConfigFileMode::kNotRequired;

USERVER_NAMESPACE_END
