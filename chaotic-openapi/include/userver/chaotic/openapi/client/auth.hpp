#pragma once

/// @file userver/chaotic/openapi/client/auth.hpp
/// @brief Auth providers that apply the credentials of security schemes to the requests of generated clients.

#include <string>
#include <string_view>
#include <unordered_map>

#include <userver/utils/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {
class Request;
}

namespace chaotic::openapi::client {

/// @brief The security requirement of an operation of a generated client
struct SecurityRequirement final {
    /// `NAME` of `userver_target_generate_openapi_client()`
    std::string_view client_name;
    /// The `securitySchemes` key
    std::string_view scheme_name;
    /// Scopes the operation requires
    utils::span<const std::string_view> scopes;
};

/// @brief Applies the credentials of security schemes to the requests of generated clients.
///
/// A provider is the place for whatever a service does to obtain credentials: read them from secdist, mint or
/// refresh tokens, pick an API key that grants the requested scopes. See
/// chaotic::openapi::client::DigestAuthProviderComponent for the provider of HTTP Digest, and
/// chaotic::openapi::client::AuthProviderComponentBase for the component that gives a provider to generated clients.
class AuthProvider {
public:
    virtual ~AuthProvider();

    /// @brief Called at service start, once for every scheme a client takes from the provider.
    ///
    /// Throw to report credentials the provider does not have, so that the service does not start without them.
    virtual void Prepare(std::string_view client_name, std::string_view scheme_name);

    /// @brief Called for every request of an operation secured by the scheme.
    virtual void Apply(clients::http::Request& request, const SecurityRequirement& requirement) = 0;
};

/// @brief Auth providers of a generated client by the `securitySchemes` key
using AuthProviders = std::unordered_map<std::string, AuthProvider*>;

/// @brief Applies the credentials of the requirement with the provider of its scheme.
/// @throws std::runtime_error if the client has no provider for the scheme.
void ApplyAuth(
    clients::http::Request& request,
    const AuthProviders& providers,
    const SecurityRequirement& requirement
);

}  // namespace chaotic::openapi::client

USERVER_NAMESPACE_END
