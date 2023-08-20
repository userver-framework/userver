#include <cstddef>
#include <userver/server/handlers/auth/auth_digest_checker_component.hpp>

#include <userver/components/component.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/dynamic_config/value.hpp>
#include <userver/utils/async.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#include <userver/server/handlers/auth/digest_algorithms.hpp>

USERVER_NAMESPACE_BEGIN

namespace component {

constexpr size_t kDefaultTTL = 10 * 1000;

AuthDigestCheckerComponent::AuthDigestCheckerComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : components::LoggableComponentBase(config, context) {
  // Reading config values from static config
  // Check for valid algorithms
  auto algorithm = config["algorithm"].As<std::string>("md5");
  if (!server::handlers::auth::kHashAlgToType.TryFindICase(algorithm).has_value()) {
    throw std::runtime_error("Algorithm is not supported: " + algorithm);
  }
  settings_.algorithm = algorithm;

  settings_.domains = config["domains"].As<std::vector<std::string>>("/");
  settings_.qops = config["qops"].As<std::vector<std::string>>("auth");
  settings_.is_proxy = config["is-proxy"].As<bool>(false);
  settings_.is_session = config["is-session"].As<bool>(false);
  settings_.nonce_ttl =
      config["nonce-ttl"].As<std::chrono::milliseconds>(kDefaultTTL);
}

AuthDigestCheckerComponent::~AuthDigestCheckerComponent() = default;

const server::handlers::auth::AuthDigestSettings&
AuthDigestCheckerComponent::GetSettings() const {
  return settings_;
}

yaml_config::Schema AuthDigestCheckerComponent::GetStaticConfigSchema() {
  return yaml_config::MergeSchemas<components::LoggableComponentBase>(R"(
type: object
description: settings for digest authentication
additionalProperties: false
properties:
    algorithm:
      type: string
      description: algorithm for hashing nonce
    domains:
      type: array
      description: domains for use
      defaultDescription: /
      items:
          type: string
          description: domain name
    qops:
      type: array
      description: qop-options
      items:
          type: string
          description: qop name
    is-proxy:
      type: boolean
      description: if set, the Proxy prefix is inserted into the header
      defaultDescription: false
    is-session:
      type: boolean
      description: enable sessions
      defaultDescription: false
    nonce-ttl:
        type: string
        description: ttl for nonce
        defaultDescription: 10s
)");
}

}  // namespace component

template <>
inline constexpr bool
    components::kHasValidate<component::AuthDigestCheckerComponent> = true;

USERVER_NAMESPACE_END
