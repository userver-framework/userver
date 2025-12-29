#pragma once

/// @file userver/server/handlers/auth/digest/auth_checker_settings_component.hpp
/// @brief @copybrief server::handlers::auth::digest::AuthCheckerSettingsComponent

#include <chrono>
#include <optional>
#include <string>

#include <userver/components/component_base.hpp>
#include <userver/dynamic_config/source.hpp>

#include "auth_checker_settings.hpp"

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth::digest {

// clang-format off

/// @brief Component that loads digest auth configuration settings from a static_config.yaml
///
/// ## Static options of server::handlers::auth::digest::AuthCheckerSettingsComponent :
/// @include{doc} scripts/docs/en/components_schema/core/src/server/handlers/auth/digest/auth_checker_settings_component.md
///
/// Options inherited from @ref components::ComponentBase :
/// @include{doc} scripts/docs/en/components_schema/core/src/components/impl/component_base.md

// clang-format on

class AuthCheckerSettingsComponent : public components::ComponentBase {
public:
    /// @ingroup userver_component_names
    /// @brief The default name of
    /// server::handlers::auth::digest::AuthCheckerSettingsComponent
    static constexpr std::string_view kName = "auth-digest-checker-settings";

    AuthCheckerSettingsComponent(
        const components::ComponentConfig& config,
        const components::ComponentContext& context
    );

    ~AuthCheckerSettingsComponent() override;

    const AuthCheckerSettings& GetSettings() const;

    static yaml_config::Schema GetStaticConfigSchema();

private:
    AuthCheckerSettings settings_;
};

}  // namespace server::handlers::auth::digest

template <>
inline constexpr bool components::kHasValidate<server::handlers::auth::digest::AuthCheckerSettingsComponent> = true;

USERVER_NAMESPACE_END
