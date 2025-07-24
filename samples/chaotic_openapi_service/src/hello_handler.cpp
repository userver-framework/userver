#include "hello_handler.hpp"

#include <userver/components/component_context.hpp>

#include <clients/test/component.hpp>

namespace samples::hello {

/// [get]
HelloHandler::HelloHandler(const components::ComponentConfig& config, const components::ComponentContext& context)
    : server::handlers::HttpHandlerBase(config, context),
      test_(context.FindComponent<::clients::test::Component>().GetClient()) {}
/// [get]

std::string
HelloHandler::HandleRequest(server::http::HttpRequest& request, server::request::RequestContext& /*request_context*/)
    const {
    auto name = request.GetArg("name");

    /// [use]
    auto response = test_.TestGet({name});
    /// [use]
    return response.body;
}

}  // namespace samples::hello
