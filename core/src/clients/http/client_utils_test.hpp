#pragma once

#include <string>

#include <fmt/format.h>

#include <userver/utest/http_client.hpp>
#include <userver/utest/simple_server.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

constexpr char kResponse200WithHeaderPattern[] =
    "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: 0\r\n{}\r\n\r\n";

struct Response200WithHeader {
  using HttpResponse = utest::SimpleServer::Response;
  using HttpRequest = utest::SimpleServer::Request;

  const std::string header;

  HttpResponse operator()(const HttpRequest&) const {
    return {
        fmt::format(kResponse200WithHeaderPattern, header),
        HttpResponse::kWriteAndClose,
    };
  }
};

}  // namespace clients::http

USERVER_NAMESPACE_END
