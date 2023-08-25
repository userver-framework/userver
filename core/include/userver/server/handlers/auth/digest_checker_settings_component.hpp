#pragma once

/// @file userver/server/handlers/auth/digest_checker_settings_component.hpp
/// @brief @copybrief server::handlers::auth::DigestCheckerSettingsComponent

#include <chrono>
#include <optional>
#include <string>

#include <userver/components/loggable_component_base.hpp>
#include <userver/dynamic_config/source.hpp>

#include "auth_digest_settings.hpp"

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {

// clang-format off

/// @brief Component that loads digest auth configuration settings from a
/// static_config.yaml
/// ## Static options:
///
/// Name       | Description                                          | Default value
/// ---------- | ---------------------------------------------------- | -------------
/// algorithm  | algorithm for hashing nonce                          | md5
/// domains    | domains for use                                      | /
/// qops       | list of qop-options                                  | auth
/// is-proxy   | if set, the Proxy prefix is inserted into the header | false
/// is-session | enable sessions                                      | false
/// nonce-ttl  | ttl for nonce                                        | 10s

// clang-format on

class DigestCheckerSettingsComponent
    : public components::LoggableComponentBase {
 public:
  /// @ingroup userver_component_names
  /// @brief The default name of
  /// server::handlers::auth::DigestCheckerSettingsComponent
  static constexpr std::string_view kName = "auth-digest-checker-settings";

  DigestCheckerSettingsComponent(const components::ComponentConfig& config,
                                 const components::ComponentContext& context);

  ~DigestCheckerSettingsComponent() override;

  const AuthDigestSettings& GetSettings() const;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  AuthDigestSettings settings_;
};

}  // namespace server::handlers::auth

template <>
inline constexpr bool components::kHasValidate<
    server::handlers::auth::DigestCheckerSettingsComponent> = true;

USERVER_NAMESPACE_END
