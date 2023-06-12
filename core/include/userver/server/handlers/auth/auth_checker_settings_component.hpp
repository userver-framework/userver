#pragma once

/// @file userver/server/handlers/auth/auth_checker_settings_component.hpp
/// @brief @copybrief components::AuthCheckerSettings

#include <userver/components/loggable_component_base.hpp>
#include <userver/storages/secdist/component.hpp>

#include "auth_checker_settings.hpp"

USERVER_NAMESPACE_BEGIN

namespace components {

// clang-format off

/// @ingroup userver_components
///
/// @brief Component that loads auth configuration settings from a
/// components::Secdist component if the latter was registered in
/// components::ComponentList.
///
/// The component does **not** have any options for service config.

// clang-format on

class AuthCheckerSettings final : public LoggableComponentBase {
 public:
  AuthCheckerSettings(const ComponentConfig&, const ComponentContext&);

  /// @ingroup userver_component_names
  /// @brief The default name of components::AuthCheckerSettings
  static constexpr std::string_view kName = "auth-checker-settings";

  const server::handlers::auth::AuthCheckerSettings& Get() const {
    return settings_;
  }

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  server::handlers::auth::AuthCheckerSettings settings_;
};

template <>
inline constexpr bool kHasValidate<AuthCheckerSettings> = true;

template <>
inline constexpr auto kConfigFileMode<AuthCheckerSettings> =
    ConfigFileMode::kNotRequired;

}  // namespace components

USERVER_NAMESPACE_END
