#pragma once

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

class DigestCheckerSettingsComponent final
    : public components::LoggableComponentBase {
 public:
  static constexpr std::string_view kName = "auth-digest-checker-settings";

  DigestCheckerSettingsComponent(const components::ComponentConfig& config,
                                 const components::ComponentContext& context);

  ~DigestCheckerSettingsComponent() final;

  const AuthDigestSettings& GetSettings() const;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  AuthDigestSettings settings_;
};

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END
