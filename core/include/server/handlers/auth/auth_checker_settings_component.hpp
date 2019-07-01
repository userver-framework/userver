#pragma once

#include <components/component_base.hpp>
#include <components/component_config.hpp>
#include <components/component_context.hpp>
#include <storages/secdist/component.hpp>

#include "auth_checker_settings.hpp"

namespace components {

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
