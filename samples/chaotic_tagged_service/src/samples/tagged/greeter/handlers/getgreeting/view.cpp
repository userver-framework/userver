#include "view.hpp"

#include <samples/tagged/greeter/client/client.hpp>

namespace samples::tagged::greeter::handlers::getgreeting {

/// [view]
Response View::Handle(Request&& request, Deps&& deps, RequestContext& /*context*/) {
    auto& upstream = deps[client::kDependency];
    try {
        // The client and the handler share the generated samples::tagged::dto::Greeting
        return Response200{upstream.getGreeting({std::move(request.name)}).body};
    } catch (const client::getgreeting::Response404& error) {
        // ... and samples::tagged::errors::Error
        Response404 response;
        response.body = error.body;
        return response;
    }
}
/// [view]

}  // namespace samples::tagged::greeter::handlers::getgreeting
