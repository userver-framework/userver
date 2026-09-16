#include <userver/server/handlers/auth/digest/auth_checker_factory.hpp>

#include <optional>
#include <stdexcept>
#include <utility>

#include <userver/storages/secdist/component.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth::digest {

namespace {

std::string ParseRealm(const HandlerAuthConfig& auth_config) {
    const auto realm = auth_config["realm"].As<std::optional<std::string>>();
    if (!realm || realm->empty()) {
        throw std::runtime_error(
            "Digest auth checker requires a non-empty realm; add 'auth.realm' to the handler static config"
        );
    }
    return *realm;
}

}  // namespace

AuthCheckerFactoryBase::AuthCheckerFactoryBase(
    const components::ComponentContext& context,
    std::string_view settings_component_name
)
    : settings_(context.FindComponent<AuthCheckerSettingsComponent>(settings_component_name).GetSettings()),
      secdist_(context.FindComponent<components::Secdist>().GetStorage())
{}

auth::AuthCheckerBasePtr AuthCheckerFactoryBase::MakeAuthChecker(const HandlerAuthConfig& auth_config) const {
    return MakeDigestAuthChecker(auth_config, ParseRealm(auth_config), settings_, secdist_.Get());
}

AuthStandaloneCheckerFactoryBase::AuthStandaloneCheckerFactoryBase(
    const components::ComponentContext& context,
    std::string_view settings_component_name
)
    : AuthCheckerFactoryBase(context, settings_component_name),
      nonce_cache_settings_(context.FindComponent<NonceCacheSettingsComponent>(settings_component_name).GetSettings())
{}

std::shared_ptr<AuthCheckerBase> AuthStandaloneCheckerFactoryBase::MakeDigestAuthChecker(
    const HandlerAuthConfig& auth_config,
    std::string realm,
    const AuthCheckerSettings& settings,
    const SecdistConfig& secdist
) const {
    return MakeStandaloneDigestAuthChecker(auth_config, std::move(realm), settings, secdist, nonce_cache_settings_);
}

}  // namespace server::handlers::auth::digest

USERVER_NAMESPACE_END
