#include "request_handlers.hpp"

namespace server {

void RequestHandlers::SetHttpRequestHandler(
    std::unique_ptr<const http::HttpRequestHandler>&& http_request_handler) {
  http_request_handler_ = std::move(http_request_handler);
}

void RequestHandlers::SetMonitorRequestHandler(
    std::unique_ptr<const http::HttpRequestHandler>&& monitor_request_handler) {
  monitor_request_handler_ = std::move(monitor_request_handler);
}

const http::HttpRequestHandler& RequestHandlers::GetHttpRequestHandler() const {
  assert(http_request_handler_);
  return *http_request_handler_;
}

const http::HttpRequestHandler& RequestHandlers::GetMonitorRequestHandler()
    const {
  assert(monitor_request_handler_);
  return *monitor_request_handler_;
}

}  // namespace server
