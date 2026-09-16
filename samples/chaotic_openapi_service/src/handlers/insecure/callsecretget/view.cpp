#include "view.hpp"

#include <utility>

#include <clients/secure/client.hpp>

namespace handlers::insecure::callsecretget {

Response View::Handle(Request&& /*request*/, Deps&& deps, RequestContext& /*context*/) {
    /// [client-use]
    auto& client = deps[::clients::secure::kDependency];

    // Credentials are not passed here: the generated client performs the
    // Digest challenge/response exchange using credentials from secdist.
    auto response = client.SecretGet();
    /// [client-use]

    return Response200{.body = std::move(response.body.greeting)};
}

}  // namespace handlers::insecure::callsecretget
