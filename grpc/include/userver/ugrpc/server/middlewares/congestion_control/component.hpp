#pragma once

/// @file userver/ugrpc/server/middlewares/congestion_control/component.hpp
/// @brief @copybrief ugrpc::server::middlewares::congestion_control::Component

#include <userver/server/congestion_control/limiter.hpp>
#include <userver/ugrpc/server/middlewares/base.hpp>
#include <userver/utils/token_bucket.hpp>

USERVER_NAMESPACE_BEGIN

/// Server congestion_control middleware
/// @see @ref scripts/docs/en/userver/grpc/server_middlewares.md
/// @see @ref ugrpc::server::middlewares::congestion_control::Component
namespace ugrpc::server::middlewares::congestion_control {

// clang-format off

/// @ingroup userver_components userver_base_classes
///
/// @brief Component for gRPC server logging
///
/// The component does **not** have any options for service config.
///
/// ## Static configuration example:
///
/// @snippet grpc/functional_tests/basic_chaos/static_config.yaml Sample grpc server congestion control middleware component config
///
/// @see @ref scripts/docs/en/userver/grpc/server_middlewares.md

// clang-format on

class Component final : public MiddlewareFactoryComponentBase,
                        public USERVER_NAMESPACE::server::congestion_control::Limitee {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of ugrpc::server::middlewares::congestion_control::Component
    static constexpr std::string_view kName = "grpc-server-congestion-control";

    Component(const components::ComponentConfig& config, const components::ComponentContext& context);

    static yaml_config::Schema GetStaticConfigSchema();

    yaml_config::Schema GetMiddlewareConfigSchema() const override;

    std::shared_ptr<const MiddlewareBase> CreateMiddleware(
        const ugrpc::server::ServiceInfo&,
        const yaml_config::YamlConfig& middleware_config
    ) const override;

    void SetLimit(std::optional<size_t> new_limit) override;

private:
    std::shared_ptr<utils::TokenBucket> rate_limit_{
        std::make_shared<utils::TokenBucket>(utils::TokenBucket::MakeUnbounded())};
};

}  // namespace ugrpc::server::middlewares::congestion_control

template <>
inline constexpr bool components::kHasValidate<ugrpc::server::middlewares::congestion_control::Component> = true;

template <>
inline constexpr auto components::kConfigFileMode<ugrpc::server::middlewares::congestion_control::Component> =
    ConfigFileMode::kNotRequired;

USERVER_NAMESPACE_END
