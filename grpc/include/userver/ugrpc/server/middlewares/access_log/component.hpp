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

/// @brief Storage to handle additional fields in access_log
/// @snippet grpc/tests/logging_test.cpp grpc log extra tag
inline const utils::AnyStorageDataTag<ugrpc::server::StorageContext, logging::LogExtra> kLogExtraTag;

/// @ingroup userver_components userver_base_classes
///
/// @brief gRPC server access log middleware component. Writes one TSKV log line per handled RPC in a static format.
/// This log is intended to be collected, parsed and stored for audit or extended statistics purposes.
///
/// This middleware is disabled and unregistered by default.
///
/// ## Static options of ugrpc::server::middlewares::access_log::Component :
/// @include{doc} scripts/docs/en/components_schema/grpc/src/ugrpc/server/middlewares/access_log/component.md
///
/// Options inherited from @ref middlewares::MiddlewareFactoryComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/middlewares/factory_component_base.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
///
/// The component name for static config is `"grpc-server-access-log"`.
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
inline constexpr auto
    components::kConfigFileMode<ugrpc::server::middlewares::access_log::Component> = ConfigFileMode::kNotRequired;

USERVER_NAMESPACE_END
