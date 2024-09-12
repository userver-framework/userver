#pragma once

#include <nghttp2/nghttp2.h>

#include <string>

USERVER_NAMESPACE_BEGIN

namespace server::http {

ssize_t NgHttp2ReadCallback(nghttp2_session*, int32_t, uint8_t*, std::size_t,
                            uint32_t*, nghttp2_data_source*, void*);

struct DataBufferSender final {
 public:
  DataBufferSender(std::string&& data);
  DataBufferSender(DataBufferSender&& o) noexcept;

  DataBufferSender(const DataBufferSender&) = delete;
  DataBufferSender& operator=(const DataBufferSender&) = delete;
  DataBufferSender& operator=(DataBufferSender&&) = delete;

  nghttp2_data_provider nghttp2_provider{};
  std::string data{};
  std::size_t sended_bytes{0};
};

class Http2Session;
class HttpResponse;

void WriteHttp2ResponseToSocket(HttpResponse& response, Http2Session& parser);

}  // namespace server::http

USERVER_NAMESPACE_END
