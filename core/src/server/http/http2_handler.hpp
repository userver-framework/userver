#pragma once

#include <server/http/http2_request_parser.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/server/request/response_base.hpp>

struct nghttp2_session;

USERVER_NAMESPACE_BEGIN

namespace server::http {

class HTTP2Handler {
 public:
  HTTP2Handler(Http2RequestParser& request_parser);

  bool Parse(const char* data, size_t size);

  void SendResponse(engine::io::Socket& socket, request::ResponseBase& response,
                    std::optional<Http2UpgradeData> upgrade_data);

 private:
  Http2RequestParser& request_parser_;
};

}  // namespace server::http

USERVER_NAMESPACE_END
