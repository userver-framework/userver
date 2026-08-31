#pragma once

// Note: this is for the purposes of tests/samples only
#include <userver/utest/using_namespace_userver.hpp>

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include <userver/formats/json/value.hpp>
#include <userver/server/handlers/auth/digest/auth_checker_factory.hpp>
#include <userver/server/handlers/auth/digest/standalone_checker.hpp>
#include <userver/storages/secdist/secdist.hpp>
#include <userver/utils/strong_typedef.hpp>

namespace samples::chaotic_openapi_service {

/// [users secdist]
/// @brief Secdist module with the demo user list.
///
/// Expected secdist JSON layout:
/// @code{.json}
/// {"digest_auth_users": {"alice": "alice-password"}}
/// @endcode
///
/// A production service stores HA1 hashes rather than passwords, see
/// @ref scripts/docs/en/userver/tutorial/digest_auth_postgres.md.
class DigestUsers final {
public:
    using Password = utils::NonLoggable<class DigestUserPasswordTag, std::string>;

    explicit DigestUsers(const formats::json::Value& doc);

    /// Returns nullptr if the user is not registered.
    const Password* FindPassword(const std::string& username) const;

private:
    std::unordered_map<std::string, Password> passwords_;
};
/// [users secdist]

/// [auth checker]
/// @brief Digest auth checker that keeps the nonce state in memory.
///
/// server::handlers::auth::digest::AuthStandaloneCheckerBase implements the
/// challenge/response exchange and nonce bookkeeping, so the only thing left to
/// do is to tell it the HA1 hash of a user.
class AuthChecker final : public server::handlers::auth::digest::AuthStandaloneCheckerBase {
public:
    AuthChecker(
        const server::handlers::auth::digest::AuthCheckerSettings& digest_settings,
        std::string realm,
        const storages::secdist::SecdistConfig& secdist_config,
        const server::handlers::auth::digest::NonceCacheSettings& nonce_cache_settings
    );

    std::optional<server::handlers::auth::digest::UserData::HA1> GetHA1(std::string_view username) const override;

private:
    // AuthCheckerBase keeps the realm private, and HA1 depends on it, so store a copy.
    const std::string realm_;
    const DigestUsers users_;
};
/// [auth checker]

/// [auth checker factory]
class CheckerFactory final : public server::handlers::auth::digest::AuthStandaloneCheckerFactoryBase {
public:
    explicit CheckerFactory(const components::ComponentContext& context);

private:
    std::shared_ptr<server::handlers::auth::digest::AuthStandaloneCheckerBase> MakeStandaloneDigestAuthChecker(
        const server::handlers::auth::HandlerAuthConfig& auth_config,
        std::string realm,
        const server::handlers::auth::digest::AuthCheckerSettings& settings,
        const storages::secdist::SecdistConfig& secdist,
        const server::handlers::auth::digest::NonceCacheSettings& nonce_cache_settings
    ) const override;
};
/// [auth checker factory]

}  // namespace samples::chaotic_openapi_service
