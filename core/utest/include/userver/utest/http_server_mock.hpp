#pragma once

#include <cstdint>
#include <map>

#include <userver/utest/simple_server.hpp>

#include <userver/clients/http/request.hpp>
#include <userver/clients/http/response.hpp>

USERVER_NAMESPACE_BEGIN

namespace utest {

class HttpServerMock {
public:
    struct HttpRequest {
        clients::http::HttpMethod method{clients::http::HttpMethod::kGet};
        std::string path;

        std::multimap<std::string, std::string> query;
        clients::http::Headers headers;
        std::string body;
    };

    struct HttpResponse {
        int response_status{200};
        clients::http::Headers headers;
        std::string body;
    };

    using HttpHandler = std::function<HttpResponse(const HttpRequest&)>;

    HttpServerMock(HttpHandler http_handler, SimpleServer::Protocol protocol = SimpleServer::kTcpIpV4);

    std::string GetBaseUrl() const;

    std::uint64_t GetConnectionsOpenedCount() const;

private:
    friend class HttpConnection;

    HttpHandler http_handler_;
    SimpleServer server_;
};

}  // namespace utest

USERVER_NAMESPACE_END
