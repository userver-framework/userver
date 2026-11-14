#pragma once

/// @file userver/chaotic/openapi/client/digest_auth.hpp
/// @brief Digest authentication credentials for chaotic-openapi generated clients.

#include <string>
#include <string_view>
#include <unordered_map>

#include <userver/chaotic/openapi/client/auth.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi::client {

/// @brief Digest auth credentials for a single securityScheme.
struct DigestAuthCredentials {
    using Password = utils::NonLoggable<class DigestPasswordTag, std::string>;

    std::string username;
    Password password;
};

/// @brief Map from securityScheme name to its digest credentials.
using DigestAuthCredentialsByScheme = std::unordered_map<std::string, DigestAuthCredentials>;

/// @brief Secdist module that loads digest auth credentials for all schemes
/// of a chaotic-openapi generated client.
///
/// Expected secdist JSON layout:
/// @code{.json}
/// {
///   "http_digest": {
///     "<client-name>": {
///       "<scheme-name>": { "username": "u", "password": "p" }
///     }
///   }
/// }
/// @endcode
class DigestAuthSecdistConfig {
public:
    explicit DigestAuthSecdistConfig(const formats::json::Value& doc);

    /// @brief Returns credentials for the given client and scheme name.
    /// @throws std::runtime_error if credentials are not found.
    const DigestAuthCredentials& GetCredentialsOrThrow(const std::string& client_name, const std::string& scheme_name)
        const;

private:
    // client_name -> scheme_name -> credentials
    std::unordered_map<std::string, DigestAuthCredentialsByScheme> credentials_;
};

/// @brief Auth provider of HTTP Digest, takes the credentials of a client and a scheme from
/// chaotic::openapi::client::DigestAuthSecdistConfig.
///
/// See chaotic::openapi::client::DigestAuthProviderComponent for the component that gives it to generated clients.
class DigestAuthProvider final : public AuthProvider {
public:
    explicit DigestAuthProvider(const DigestAuthSecdistConfig& credentials);

    /// @throws std::runtime_error if the secdist has no credentials for the client and the scheme
    void Prepare(std::string_view client_name, std::string_view scheme_name) override;

    void Apply(clients::http::Request& request, const SecurityRequirement& requirement) override;

private:
    const DigestAuthSecdistConfig& credentials_;
};

}  // namespace chaotic::openapi::client

USERVER_NAMESPACE_END
