#pragma once

/// @file userver/server/handlers/auth/digest/auth_checker_factory.hpp
/// @brief Reusable factories for server-side HTTP Digest authentication.

#include <memory>
#include <string>
#include <string_view>

#include <userver/components/component_context.hpp>
#include <userver/server/handlers/auth/auth_checker_factory.hpp>
#include <userver/server/handlers/auth/digest/auth_checker_base.hpp>
#include <userver/server/handlers/auth/digest/auth_checker_settings_component.hpp>
#include <userver/server/handlers/auth/digest/nonce_cache_settings_component.hpp>
#include <userver/server/handlers/auth/digest/standalone_checker.hpp>
#include <userver/storages/secdist/secdist.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth::digest {

/// @brief Factory base that owns the common Digest checker configuration wiring.
class AuthCheckerFactoryBase : public auth::AuthCheckerFactoryBase {
public:
    explicit AuthCheckerFactoryBase(
        const components::ComponentContext& context,
        std::string_view settings_component_name = AuthCheckerSettingsComponent::kName
    );

    auth::AuthCheckerBasePtr MakeAuthChecker(const HandlerAuthConfig& auth_config) const final;

protected:
    virtual std::shared_ptr<AuthCheckerBase> MakeDigestAuthChecker(
        const HandlerAuthConfig& auth_config,
        std::string realm,
        const AuthCheckerSettings& settings,
        const SecdistConfig& secdist
    ) const = 0;

private:
    const AuthCheckerSettings& settings_;
    storages::secdist::Secdist& secdist_;
};

/// @brief Factory base for Digest checkers that keep nonce state in memory.
class AuthStandaloneCheckerFactoryBase : public AuthCheckerFactoryBase {
public:
    explicit AuthStandaloneCheckerFactoryBase(
        const components::ComponentContext& context,
        std::string_view settings_component_name = NonceCacheSettingsComponent::kName
    );

protected:
    virtual std::shared_ptr<AuthStandaloneCheckerBase> MakeStandaloneDigestAuthChecker(
        const HandlerAuthConfig& auth_config,
        std::string realm,
        const AuthCheckerSettings& settings,
        const SecdistConfig& secdist,
        const NonceCacheSettings& nonce_cache_settings
    ) const = 0;

private:
    std::shared_ptr<AuthCheckerBase> MakeDigestAuthChecker(
        const HandlerAuthConfig& auth_config,
        std::string realm,
        const AuthCheckerSettings& settings,
        const SecdistConfig& secdist
    ) const final;

    const NonceCacheSettings& nonce_cache_settings_;
};

}  // namespace server::handlers::auth::digest

USERVER_NAMESPACE_END
