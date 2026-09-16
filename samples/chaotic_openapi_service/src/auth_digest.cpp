#include "auth_digest.hpp"

#include <utility>

#include <fmt/format.h>

#include <userver/components/component_context.hpp>
#include <userver/crypto/hash.hpp>
#include <userver/utils/algo.hpp>

namespace samples::chaotic_openapi_service {

DigestUsers::DigestUsers(const formats::json::Value& doc) {
    const auto& users = doc["digest_auth_users"];
    if (users.IsMissing() || users.IsNull()) {
        return;
    }

    for (const auto& [username, password] : Items(users)) {
        passwords_.emplace(username, Password{password.As<std::string>()});
    }
}

const DigestUsers::Password* DigestUsers::FindPassword(const std::string& username) const {
    return utils::FindOrNullptr(passwords_, username);
}

/// [get ha1]
AuthChecker::AuthChecker(
    const server::handlers::auth::digest::AuthCheckerSettings& digest_settings,
    std::string realm,
    const storages::secdist::SecdistConfig& secdist_config,
    const server::handlers::auth::digest::NonceCacheSettings& nonce_cache_settings
)
    : server::handlers::auth::digest::AuthStandaloneCheckerBase(
          digest_settings,
          std::string{realm},
          secdist_config,
          nonce_cache_settings
      ),
      realm_(std::move(realm)),
      // A copy of the secdist snapshot data: the checker outlives a single snapshot.
      users_(secdist_config.Get<DigestUsers>())
{}

std::optional<server::handlers::auth::digest::UserData::HA1> AuthChecker::GetHA1(std::string_view username) const {
    const auto* password = users_.FindPassword(std::string{username});
    if (password == nullptr) {
        // Unregistered user.
        return std::nullopt;
    }

    // HA1 uses the algorithm selected by the server. This compatibility sample
    // uses MD5; production services should use the strongest mutually supported algorithm.
    return server::handlers::auth::digest::UserData::HA1{
        crypto::hash::weak::Md5(fmt::format("{}:{}:{}", username, realm_, password->GetUnderlying()))
    };
}
/// [get ha1]

/// [auth checker factory definition]
CheckerFactory::CheckerFactory(const components::ComponentContext& context)
    : AuthStandaloneCheckerFactoryBase(context)
{}

std::shared_ptr<server::handlers::auth::digest::AuthStandaloneCheckerBase>
CheckerFactory::MakeStandaloneDigestAuthChecker(
    const server::handlers::auth::HandlerAuthConfig&,
    std::string realm,
    const server::handlers::auth::digest::AuthCheckerSettings& settings,
    const storages::secdist::SecdistConfig& secdist,
    const server::handlers::auth::digest::NonceCacheSettings& nonce_cache_settings
) const {
    return std::make_shared<AuthChecker>(settings, std::move(realm), secdist, nonce_cache_settings);
}
/// [auth checker factory definition]

}  // namespace samples::chaotic_openapi_service
