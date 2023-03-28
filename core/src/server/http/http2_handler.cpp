#include "http2_handler.hpp"

#include <server/http/http_response_writer.hpp>
#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

HTTP2Handler::HTTP2Handler(Http2RequestParser& request_parser)
    : request_parser_(request_parser) {}

bool HTTP2Handler::Parse(const char* data, size_t size) {
  return request_parser_.Parse(data, size);
}

void HTTP2Handler::SendResponse(engine::io::Socket& socket,
                                request::ResponseBase& response,
                                std::optional<Http2UpgradeData> upgrade_data) {
  LOG_DEBUG() << "Send http2 response";
  response.SendResponseHttp2(socket,
                             request_parser_.SessionData()->GetSession(),
                             std::move(upgrade_data));
}

}  // namespace server::http

USERVER_NAMESPACE_END
