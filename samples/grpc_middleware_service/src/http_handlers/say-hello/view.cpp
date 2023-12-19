#include "view.hpp"

#include <userver/components/component.hpp>
#include <userver/components/loggable_component_base.hpp>

namespace samples::grpc::auth {

GreeterHttpHandler::GreeterHttpHandler(
    const components::ComponentConfig& config,
    const components::ComponentContext& context)
    : HttpHandlerBase(config, context),
      grpc_greeter_client_(context.FindComponent<GreeterClient>()) {}

std::string GreeterHttpHandler::HandleRequestThrow(
    const server::http::HttpRequest& request,
    server::request::RequestContext&) const {
  return grpc_greeter_client_.SayHello(request.RequestBody());
}

}  // namespace samples::grpc::auth
