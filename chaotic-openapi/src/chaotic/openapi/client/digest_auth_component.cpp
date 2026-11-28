#include <userver/chaotic/openapi/client/digest_auth_component.hpp>

#include <userver/components/component_context.hpp>
#include <userver/storages/secdist/component.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi::client {

DigestAuthProviderComponent::DigestAuthProviderComponent(
    const components::ComponentConfig& config,
    const components::ComponentContext& context
)
    : AuthProviderComponentBase(config, context),
      provider_(context.FindComponent<components::Secdist>().Get().Get<DigestAuthSecdistConfig>())
{}

AuthProvider& DigestAuthProviderComponent::GetAuthProvider() { return provider_; }

}  // namespace chaotic::openapi::client

USERVER_NAMESPACE_END
