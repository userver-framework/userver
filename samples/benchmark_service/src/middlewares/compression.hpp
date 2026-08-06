#pragma once

#include <userver/utest/using_namespace_userver.hpp>

#include <string_view>

#include <userver/server/middlewares/http_middleware_base.hpp>

namespace userver_httparena::middlewares {
class CompressionMiddleware final : public server::middlewares::HttpMiddlewareBase {
public:
    static constexpr std::string_view kName = "compression-middleware";

    explicit CompressionMiddleware(const server::handlers::HttpHandlerBase&);

    void HandleRequest(server::http::HttpRequest& request, server::request::RequestContext& context) const final;
};
}  // namespace userver_httparena::middlewares
