#include <userver/chaotic/openapi/client/auth.hpp>

#include <stdexcept>

#include <fmt/format.h>

#include <userver/utils/algo.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::openapi::client {

AuthProvider::~AuthProvider() = default;

void AuthProvider::Prepare(std::string_view /*client_name*/, std::string_view /*scheme_name*/) {}

void ApplyAuth(
    clients::http::Request& request,
    const AuthProviders& providers,
    const SecurityRequirement& requirement
) {
    const auto* provider = utils::FindOrNullptr(providers, std::string{requirement.scheme_name});
    if (provider == nullptr || *provider == nullptr) {
        throw std::runtime_error{fmt::format(
            "chaotic-openapi client '{}' has no auth provider for security scheme '{}'",
            requirement.client_name,
            requirement.scheme_name
        )};
    }
    (*provider)->Apply(request, requirement);
}

}  // namespace chaotic::openapi::client

USERVER_NAMESPACE_END
