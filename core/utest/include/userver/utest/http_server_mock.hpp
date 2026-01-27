#pragma once

/// @file userver/utest/http_server_mock.hpp
/// @brief @copybrief utest::HttpServerMock

#include <cstdint>
#include <map>

#include <userver/utest/simple_server.hpp>

#include <userver/clients/http/request.hpp>
#include <userver/clients/http/response.hpp>

USERVER_NAMESPACE_BEGIN

namespace utest {

/// @ingroup userver_utest
///
/// @brief Toy HTTP server for testing; for raw TCP or TLS testing use utest::SimpleServer.
///
/// In constructor opens specified ports in localhost address and listens on
/// them. On each HTTP request calls user callback.
///
/// ## Example usage:
/// @snippet core/utest/src/utest/http_server_mock_test.cpp  Sample HttpServerMock usage
class HttpServerMock {
public:
    /// Structure with HTTP request that is passed to the HttpHandler callback
    struct HttpRequest {
        clients::http::HttpMethod method{clients::http::HttpMethod::kGet};
        std::string path;

        std::multimap<std::string, std::string> query;
        clients::http::Headers headers;
        std::string body;
    };

    /// Structure with HTTP response to return from the HttpHandler callback
    struct HttpResponse {
        int response_status{200};
        clients::http::Headers headers;
        std::string body;
    };

    /// Callback that is invoked on each HTTP request
    using HttpHandler = std::function<HttpResponse(const HttpRequest&)>;

    HttpServerMock(HttpHandler http_handler, SimpleServer::Protocol protocol = SimpleServer::kTcpIpV4);

    /// Returns URL to the server, for example 'http://127.0.0.1:8080'
    std::string GetBaseUrl() const;

    std::uint64_t GetConnectionsOpenedCount() const;

private:
    friend class HttpConnection;

    HttpHandler http_handler_;
    SimpleServer server_;
};

}  // namespace utest

USERVER_NAMESPACE_END
