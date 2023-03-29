#include "http1_handler.hpp"

USERVER_NAMESPACE_BEGIN

namespace server::http {

HTTP1Handler::HTTP1Handler(http::HttpRequestParser& http_parser)
    : http1_parser_(http_parser) {}

bool HTTP1Handler::Parse(const char* data, size_t size) {
  return http1_parser_.Parse(data, size);
}

void HTTP1Handler::SendResponse(engine::io::Socket& socket,
                                request::ResponseBase& response) const {
  response.SendResponse(socket);
}

}  // namespace server::http

USERVER_NAMESPACE_END
