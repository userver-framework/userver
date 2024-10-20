#pragma once

#include <userver/server/handlers/http_handler_base.hpp>

namespace chaos {

class HttpServerWithExceptionHandler final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "handler-chaos-httpserver-with-exception";

    HttpServerWithExceptionHandler(
        const components::ComponentConfig& config,
        const components::ComponentContext& context
    )
        : HttpHandlerBase(config, context) {}

    std::string HandleRequestThrow(const server::http::HttpRequest&, server::request::RequestContext&) const override {
        throw std::runtime_error("some runtime error");
    }
};

}  // namespace chaos
