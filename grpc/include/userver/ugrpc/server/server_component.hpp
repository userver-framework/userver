#pragma once

/// @file userver/ugrpc/server/server_component.hpp
/// @brief @copybrief ugrpc::server::ServerComponent

#include <userver/components/component_base.hpp>

#include <userver/ugrpc/server/server.hpp>

USERVER_NAMESPACE_BEGIN

namespace ugrpc::server {

namespace impl {
struct ServiceDefaults;
}  // namespace impl

/// @ingroup userver_components
///
/// @brief Component that configures and manages the gRPC server.
///
/// ## Static options of ugrpc::server::ServerComponent :
/// @include{doc} scripts/docs/en/components_schema/grpc/src/ugrpc/server/server_component.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md
///
/// The component name for static config is `"grpc-server"`.
///
/// @see https://grpc.github.io/grpc/core/group__grpc__arg__keys.html
class ServerComponent final : public components::ComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of ugrpc::server::ServerComponent
    static constexpr std::string_view kName = "grpc-server";

    ServerComponent(const components::ComponentConfig& config, const components::ComponentContext& context);

    ~ServerComponent() override;

    /// @returns The contained Server instance
    /// @note All configuration must be performed at the components loading stage
    Server& GetServer() noexcept;

    /// @cond
    ServiceConfig ParseServiceConfig(
        const components::ComponentConfig& config,
        const components::ComponentContext& context
    );
    /// @endcond

    static yaml_config::Schema GetStaticConfigSchema();

private:
    void OnAllComponentsLoaded() override;

    void OnAllComponentsAreStopping() override;

    Server server_;
    std::unique_ptr<impl::ServiceDefaults> service_defaults_;
};

}  // namespace ugrpc::server

template <>
inline constexpr bool components::kHasValidate<ugrpc::server::ServerComponent> = true;

USERVER_NAMESPACE_END
