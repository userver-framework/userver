#pragma once

/// @file
/// userver/server/handlers/auth/digest/digest_checker_settings_component.hpp
/// @brief @copybrief
/// server::handlers::auth::digest::AuthCheckerSettingsComponent

#include <chrono>
#include <optional>
#include <string>

#include <userver/components/loggable_component_base.hpp>
#include <userver/dynamic_config/source.hpp>

#include "auth_checker_settings.hpp"

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth::digest {

// clang-format off

/// @brief Component that loads digest auth configuration settings from a
/// static_config.yaml
/// ## Static options:
///
/// Name       | Description                                    | Default value
/// ---------- | ---------------------------------------------- | -------------
/// algorithm  | algorithm for hashing nonce                    | sha256
/// domains    | list of URIs, that define the protection space | /
/// qops       | list of supported qop-options. Use `auth` for authentication and `auth-in` for authentication with integrity protection | auth
/// is-proxy   | indicates that the server is a proxy server. If set, the Proxy prefix is inserted into the header | false
/// is-session | activate session algorithm (md5-sess, sha256-sess or sha512-sess) | false
/// nonce-ttl  | ttl for nonce | 10s

// clang-format on

class AuthCheckerSettingsComponent : public components::LoggableComponentBase {
 public:
  /// @ingroup userver_component_names
  /// @brief The default name of
  /// server::handlers::auth::digest::AuthCheckerSettingsComponent
  static constexpr std::string_view kName = "auth-digest-checker-settings";

  AuthCheckerSettingsComponent(const components::ComponentConfig& config,
                               const components::ComponentContext& context);

  ~AuthCheckerSettingsComponent() override;

  const AuthCheckerSettings& GetSettings() const;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  AuthCheckerSettings settings_;
};

}  // namespace server::handlers::auth::digest

template <>
inline constexpr bool components::kHasValidate<
    server::handlers::auth::digest::AuthCheckerSettingsComponent> = true;

USERVER_NAMESPACE_END
