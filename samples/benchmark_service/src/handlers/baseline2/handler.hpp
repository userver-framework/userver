#pragma once

#include <userver/utest/using_namespace_userver.hpp>

#include <userver/server/handlers/http_handler_base.hpp>

namespace userver_httparena::baseline2 {
class Handler final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "baseline2-handler";

    using HttpHandlerBase::HttpHandlerBase;

    std::string HandleRequestThrow(const server::http::HttpRequest& request, server::request::RequestContext&)
        const final;
};
}  // namespace userver_httparena::baseline2
