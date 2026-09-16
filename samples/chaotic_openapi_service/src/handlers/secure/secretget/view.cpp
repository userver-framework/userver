#include "view.hpp"

namespace handlers::secure::secretget {

/// [view-impl]
Response View::Handle(Request&& /*request*/, Deps&& /*deps*/, RequestContext& /*context*/) {
    // The request is already authenticated here: chaotic-openapi has generated
    // `auth: {types: [myDigestScheme]}` for this handler into config.chaotic.yaml,
    // so the digest auth checker has run before the handler.
    return Response200{.body = {.greeting = "Hello, authenticated user!"}};
}
/// [view-impl]

}  // namespace handlers::secure::secretget
