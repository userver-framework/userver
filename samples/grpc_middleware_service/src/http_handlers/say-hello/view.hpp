#pragma once

#include <client/view.hpp>

#include <userver/server/handlers/http_handler_base.hpp>

namespace samples::grpc::auth {

// Our Python tests use HTTP for all the samples, so we add an HTTP handler,
// through which we test both the client side and the server side.
class GreeterHttpHandler final : public server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "greeter-http-handler";

  GreeterHttpHandler(const components::ComponentConfig& config,
                     const components::ComponentContext& context);

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&) const override;

 private:
  GreeterClient& grpc_greeter_client_;
};

}  // namespace samples::grpc::auth
