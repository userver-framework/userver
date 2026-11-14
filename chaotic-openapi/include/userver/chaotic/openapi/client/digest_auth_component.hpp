#pragma once

/// @file userver/chaotic/openapi/client/digest_auth_component.hpp
/// @brief @copybrief chaotic::openapi::client::DigestAuthProviderComponent

#include <string_view>

#include <userver/chaotic/openapi/client/auth_component.hpp>
#include <userver/chaotic/openapi/client/digest_auth.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi::client {

/// @brief Component that gives chaotic::openapi::client::DigestAuthProvider to generated clients.
///
/// Takes the credentials from secdist, see chaotic::openapi::client::DigestAuthSecdistConfig. A generated client
/// takes it for a digest scheme unless its static config names another provider in `auth-providers`.
///
/// The component has no static config options, it requires components::Secdist.
class DigestAuthProviderComponent final : public AuthProviderComponentBase {
public:
    /// The default name of the component, the one generated clients take for digest schemes
    static constexpr std::string_view kName = "chaotic-digest-auth-provider";

    DigestAuthProviderComponent(const components::ComponentConfig& config, const components::ComponentContext& context);

    AuthProvider& GetAuthProvider() override;

private:
    DigestAuthProvider provider_;
};

}  // namespace chaotic::openapi::client

template <>
inline constexpr auto components::kConfigFileMode<chaotic::openapi::client::DigestAuthProviderComponent> =
    components::ConfigFileMode::kNotRequired;

USERVER_NAMESPACE_END
