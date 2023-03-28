#pragma once

#include <server/http/http_handler.hpp>
#include <server/http/http_request_parser.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/server/request/response_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

class HTTP1Handler : public HTTPHandler {
 public:
  HTTP1Handler(http::HttpRequestParser& http_parser);

  bool Parse(const char* data, size_t size) override;

  void SendResponse(engine::io::Socket& socket,
                    request::ResponseBase& response) const override;

 private:
  http::HttpRequestParser& http1_parser_;
};

}  // namespace server::http

USERVER_NAMESPACE_END
