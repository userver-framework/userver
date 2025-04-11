#pragma once

#include <string_view>

#include <userver/components/component.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/utils/text.hpp>

#include <greeter_client.hpp>

namespace samples {

// To demonstrate the possibility to use gRPC clients separately from gRPC handlers,
// we use a simple HTTP handler.
class CallGreeterClientTestHandler final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "greeter-http-handler";

    CallGreeterClientTestHandler(const components::ComponentConfig& config, const components::ComponentContext& context)
        : HttpHandlerBase(config, context),
          grpc_greeter_client_(context.FindComponent<GreeterClientComponent>().GetClient()) {}

    std::string HandleRequest(server::http::HttpRequest& request, server::request::RequestContext&) const override {
        const auto& arg_case = request.GetArg("case");
        request.GetHttpResponse().SetContentType(http::content_type::kTextPlain);

        if (arg_case == "say_hello") {
            return grpc_greeter_client_.SayHello(request.RequestBody());
        } else if (arg_case == "say_hello_response_stream") {
            std::string response =
                utils::text::Join(grpc_greeter_client_.SayHelloResponseStream(request.RequestBody()), "\n");
            response.append("\n");
            return response;
        } else if (arg_case == "say_hello_request_stream") {
            const auto names = utils::text::SplitIntoStringViewVector(request.RequestBody(), "\n");
            return grpc_greeter_client_.SayHelloRequestStream(names);
        } else if (arg_case == "say_hello_streams") {
            const auto names = utils::text::SplitIntoStringViewVector(request.RequestBody(), "\n");
            std::string response = utils::text::Join(grpc_greeter_client_.SayHelloStreams(names), "\n");
            response.append("\n");
            return response;
        } else {
            throw server::handlers::CustomHandlerException(server::handlers::HandlerErrorCode::kResourceNotFound);
        }
    }

private:
    const GreeterClient& grpc_greeter_client_;
};

}  // namespace samples
