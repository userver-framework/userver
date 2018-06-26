#pragma once

#include <memory>

#include <server/http/http_request_handler.hpp>

namespace server {

class RequestHandlers {
 public:
  void SetHttpRequestHandler(
      std::unique_ptr<const http::HttpRequestHandler>&& http_request_handler);
  void SetMonitorRequestHandler(
      std::unique_ptr<const http::HttpRequestHandler>&&
          monitor_request_handler);

  const http::HttpRequestHandler& GetHttpRequestHandler() const;
  const http::HttpRequestHandler& GetMonitorRequestHandler() const;

 private:
  std::unique_ptr<const http::HttpRequestHandler> http_request_handler_;
  std::unique_ptr<const http::HttpRequestHandler> monitor_request_handler_;
};

}  // namespace server
