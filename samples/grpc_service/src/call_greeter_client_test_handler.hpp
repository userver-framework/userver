#pragma once

#include <string>
#include <string_view>

#include <userver/components/component.hpp>
#include <userver/server/handlers/http_handler_base.hpp>

#include <greeter_client.hpp>

namespace samples {

// Our Python tests use HTTP for all the samples, so we add an HTTP handler,
// through which we test both the client side and the server side.
class CallGreeterClientTestHandler final
    : public server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "greeter-http-handler";

  CallGreeterClientTestHandler(const components::ComponentConfig& config,
                               const components::ComponentContext& context)
      : HttpHandlerBase(config, context),
        grpc_greeter_client_(
            context.FindComponent<GreeterClientComponent>().GetClient()) {}

  std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&) const override {
    return grpc_greeter_client_.SayHello(request.RequestBody());
  }

 private:
  const GreeterClient& grpc_greeter_client_;
};

}  // namespace samples
