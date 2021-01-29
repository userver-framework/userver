#pragma once

#include <utest/simple_server.hpp>

#include <clients/http/request.hpp>
#include <clients/http/response.hpp>

namespace testing {

class HttpServerMock {
 public:
  struct HttpRequest {
    clients::http::HttpMethod method{clients::http::HttpMethod::kGet};
    std::string path;

    std::unordered_map<std::string, std::string> query;
    clients::http::Headers headers;
    std::string body;
  };

  struct HttpResponse {
    int response_status;
    clients::http::Headers headers;
    std::string body;
  };

  using HttpHandler = std::function<HttpResponse(const HttpRequest&)>;

  HttpServerMock(HttpHandler http_handler,
                 SimpleServer::Protocol protocol = SimpleServer::kTcpIpV4);

  std::string GetBaseUrl() const;

 private:
  SimpleServer::OnRequest OnNewConnection();

 private:
  HttpHandler http_handler_;
  SimpleServer server_;
};

}  // namespace testing
