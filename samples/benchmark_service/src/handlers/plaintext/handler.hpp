#pragma once

#include <userver/utest/using_namespace_userver.hpp>

#include <userver/server/handlers/http_handler_base.hpp>

namespace userver_httparena::plaintext {
class Handler final : public server::handlers::HttpHandlerBase {
public:
    static constexpr std::string_view kName = "plaintext-handler";

    using HttpHandlerBase::HttpHandlerBase;

    std::string HandleRequestThrow(const server::http::HttpRequest& request, server::request::RequestContext&)
        const final;

    static std::string GetResponse();
};
}  // namespace userver_httparena::plaintext
