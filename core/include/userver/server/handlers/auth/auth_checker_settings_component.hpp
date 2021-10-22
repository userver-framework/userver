#pragma once

/// @file userver/server/handlers/auth/auth_checker_settings_component.hpp
/// @brief @copybrief components::AuthCheckerSettings

#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>
#include <userver/components/loggable_component_base.hpp>
#include <userver/storages/secdist/component.hpp>

#include "auth_checker_settings.hpp"

USERVER_NAMESPACE_BEGIN

namespace components {

// clang-format off

/// @ingroup userver_components
///
/// @brief Component that loads auth configuration settings from a
/// components::Secdist componenet if the latter was registered in
/// components::ComponentList.
///
/// The component does **not** have any options for service config.

// clang-format on

class AuthCheckerSettings final : public LoggableComponentBase {
 public:
  AuthCheckerSettings(const ComponentConfig&, const ComponentContext&);

  static constexpr const char* kName = "auth-checker-settings";

  const server::handlers::auth::AuthCheckerSettings& Get() const {
    return settings_;
  }

 private:
  server::handlers::auth::AuthCheckerSettings settings_;
};

}  // namespace components

USERVER_NAMESPACE_END
