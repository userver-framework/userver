#include <userver/server/http/http_response_body_stream.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

ResponseBodyStream::ResponseBodyStream(
    server::http::HttpResponse::Queue::Producer&& queue_producer,
    server::http::HttpResponse& http_response)
    : queue_producer_(std::move(queue_producer)),
      http_response_(http_response) {}

void ResponseBodyStream::PushBodyChunk(std::string&& chunk) {
  UASSERT_MSG(headers_ended_,
              "SetEndOfHeaders() was not called before PushBodyChunk()");
  queue_producer_.Push(std::move(chunk));
}

void ResponseBodyStream::SetHeader(const std::string& name,
                                   const std::string& value) {
  http_response_.SetHeader(name, value);
}

void ResponseBodyStream::SetEndOfHeaders() { headers_ended_ = true; }

void ResponseBodyStream::SetStatusCode(int status_code) {
  http_response_.SetStatus(static_cast<server::http::HttpStatus>(status_code));
}

void ResponseBodyStream::SetStatusCode(HttpStatus status) {
  http_response_.SetStatus(status);
}

}  // namespace server::http

USERVER_NAMESPACE_END
