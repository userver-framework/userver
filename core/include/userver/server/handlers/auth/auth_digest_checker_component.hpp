#pragma once

#include <chrono>
#include <optional>
#include <string>

#include <userver/components/loggable_component_base.hpp>
#include <userver/dynamic_config/source.hpp>

#include "auth_digest_settings.hpp"

USERVER_NAMESPACE_BEGIN

namespace component {

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

class AuthDigestCheckerComponent final
    : public components::LoggableComponentBase {
 public:
  static constexpr std::string_view kName = "auth-digest-checker-settings";

  AuthDigestCheckerComponent(const components::ComponentConfig& config,
                             const components::ComponentContext& context);

  ~AuthDigestCheckerComponent() final;

  const server::handlers::auth::AuthDigestSettings& GetSettings() const;

  static yaml_config::Schema GetStaticConfigSchema();

 private:
  server::handlers::auth::AuthDigestSettings settings_;
};

}  // namespace component

USERVER_NAMESPACE_END
