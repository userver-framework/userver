#include <userver/server/handlers/auth/digest/auth_checker_settings_component.hpp>

#include <cstddef>

#include <userver/components/component.hpp>
#include <userver/dynamic_config/storage/component.hpp>
#include <userver/dynamic_config/value.hpp>
#include <userver/server/handlers/auth/digest/types.hpp>
#include <userver/utils/async.hpp>
#include <userver/yaml_config/merge_schemas.hpp>

#ifndef ARCADIA_ROOT
#include "generated/src/server/handlers/auth/digest/auth_checker_settings_component.yaml.hpp"  // Y_IGNORE
#endif

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth::digest {

constexpr size_t kDefaultTtlMs = 10 * 1000;

AuthCheckerSettingsComponent::AuthCheckerSettingsComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
)
    : components::ComponentBase(config, context) {
    // Reading config values from static config
    // Check for valid algorithms
    auto algorithm = config["algorithm"].As<std::string>("sha256");
    if (!kHashAlgToType.TryFindICase(algorithm).has_value()) {
        throw std::runtime_error("Algorithm is not supported: " + algorithm);
    }
    settings_.algorithm = algorithm;

    settings_.domains = config["domains"].As<std::vector<std::string>>(std::vector<std::string>{"/"});
    settings_.qops = config["qops"].As<std::vector<std::string>>(std::vector<std::string>{"auth"});
    // Check for valid qops
    for (const auto& qop : settings_.qops) {
        if (!kQopToType.TryFindICase(qop).has_value()) {
            throw std::runtime_error("Qop is not supported: " + qop);
        }
    }

    settings_.is_proxy = config["is-proxy"].As<bool>(false);
    settings_.is_session = config["is-session"].As<bool>(false);
    settings_.nonce_ttl = config["nonce-ttl"].As<std::chrono::milliseconds>(kDefaultTtlMs);
}

AuthCheckerSettingsComponent::~AuthCheckerSettingsComponent() = default;

const AuthCheckerSettings& AuthCheckerSettingsComponent::GetSettings() const { return settings_; }

yaml_config::Schema AuthCheckerSettingsComponent::GetStaticConfigSchema() {
    return yaml_config::MergeSchemasFromResource<
        components::ComponentBase>("src/server/handlers/auth/digest/auth_checker_settings_component.yaml");
}

}  // namespace server::handlers::auth::digest

USERVER_NAMESPACE_END
