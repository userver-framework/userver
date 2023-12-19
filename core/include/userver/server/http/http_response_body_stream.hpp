#pragma once

#include <string>

#include <userver/server/http/http_response.hpp>
#include <userver/server/request/response_base.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers {
class HttpHandlerBase;
}

namespace server::http {

class ResponseBodyStream final {
 public:
  ResponseBodyStream(ResponseBodyStream&&) = default;

  // Send a chunk of response data. It may NOT generate
  // exactly one HTTP chunk per call to PushBodyChunk().
  void PushBodyChunk(std::string&& chunk, engine::Deadline deadline);

  void SetHeader(const std::string&, const std::string&);

  void SetHeader(std::string_view, const std::string&);

  void SetEndOfHeaders();

  void SetStatusCode(int status_code);

  void SetStatusCode(HttpStatus status);

 private:
  friend class server::handlers::HttpHandlerBase;

  ResponseBodyStream(
      server::http::HttpResponse::Queue::Producer&& queue_producer,
      server::http::HttpResponse& http_response);

  bool headers_ended_{false};
  HttpResponse::Queue::Producer queue_producer_;
  server::http::HttpResponse& http_response_;
};

}  // namespace server::http

USERVER_NAMESPACE_END
