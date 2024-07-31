#include "hello_handler.hpp"

#include <userver/components/component_context.hpp>

namespace samples::hello {
HelloHandler::HelloHandler(const components::ComponentConfig& config,
                           const components::ComponentContext& context)
    : server::handlers::HttpHandlerBase(config, context),
      upy_(context.FindComponent<upython::Component>()) {}

std::string HelloHandler::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext& /*request_context*/) const {
  return upy_.RunScript();
}

}  // namespace samples::hello
