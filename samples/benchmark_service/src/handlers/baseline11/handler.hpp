#pragma once

#include <userver/utest/using_namespace_userver.hpp>

#include <userver/server/handlers/http_handler_base.hpp>

namespace userver_httparena::baseline11 {
class Handler final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "baseline11-handler";

    using HttpHandlerBase::HttpHandlerBase;

    std::string HandleRequestThrow(const server::http::HttpRequest& request, server::request::RequestContext&)
        const final;

    static std::string GetResponse(const std::string& a, const std::string& b, const std::string& body);
};
}  // namespace userver_httparena::baseline11
