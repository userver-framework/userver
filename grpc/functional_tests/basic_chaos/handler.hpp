#pragma once

#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/utest/using_namespace_userver.hpp>
#include <userver/utils/text.hpp>

#include "client.hpp"

namespace samples {

class GreeterHttpHandler final : public server::handlers::HttpHandlerBase {
 public:
  static constexpr std::string_view kName = "greeter-http-handler";

  GreeterHttpHandler(const components::ComponentConfig& config,
                     const components::ComponentContext& context)
      : HttpHandlerBase(config, context),
        grpc_greeter_client_(context.FindComponent<GreeterClient>()) {}

  inline std::string HandleRequestThrow(
      const server::http::HttpRequest& request,
      server::request::RequestContext&) const override {
    const auto arg_case = request.GetArg("case");
    const auto arg_timeout = request.GetArg("timeout");
    bool is_small_timeout = false;
    if (arg_timeout == "small") {
      is_small_timeout = true;
    }

    if (arg_case == "say_hello") {
      return grpc_greeter_client_.SayHello(request.RequestBody(),
                                           is_small_timeout);
    } else if (arg_case == "say_hello_response_stream") {
      return grpc_greeter_client_.SayHelloResponseStream(request.RequestBody(),
                                                         is_small_timeout);
    } else if (arg_case == "say_hello_request_stream") {
      const auto names = utils::text::Split(request.RequestBody(), "\n");
      return grpc_greeter_client_.SayHelloRequestStream(names,
                                                        is_small_timeout);
    } else if (arg_case == "say_hello_streams") {
      const auto names = utils::text::Split(request.RequestBody(), "\n");
      return grpc_greeter_client_.SayHelloStreams(names, is_small_timeout);
    } else if (arg_case == "say_hello_indept_streams") {
      const auto names = utils::text::Split(request.RequestBody(), "\n");
      return grpc_greeter_client_.SayHelloIndependentStreams(names,
                                                             is_small_timeout);
    }
    return "Case not found";
  }

 private:
  GreeterClient& grpc_greeter_client_;
};

}  // namespace samples
