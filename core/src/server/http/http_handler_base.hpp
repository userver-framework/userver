#pragma once

#include "http1_handler.hpp"
#include "http2_handler.hpp"

#include <server/http/http_request_parser.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/server/request/response_base.hpp>

#include <nghttp2/nghttp2.h>

USERVER_NAMESPACE_BEGIN

namespace server::http {

enum class HttpProtocol {
  kHTTP1 = 0,
  kHTTP2 = 1,
};

class HTTPHandlerBase {
 public:
  HTTPHandlerBase(const HandlerInfoIndex& handler_info_index,
                  const request::HttpRequestConfig& request_config,
                  OnNewRequestCb&& on_new_request_cb, net::ParserStats& stats,
                  request::ResponseDataAccounter& data_accounter,
                  http::Http2UpgradeData upgrade_data);

  bool Parse(const char* data, size_t size);
  void SendResponse(engine::io::Socket& socket,
                    request::ResponseBase& response);
  void UpgradeToHttp2(const std::string& settings);

  static bool CheckUpgradeHeaders(const http::HttpRequestImpl& http_request);
  void CheckUpgrade(const http::HttpRequestImpl& http_request);

 private:
  const HandlerInfoIndex& handler_info_index_;
  const HttpRequestConstructor::Config request_constructor_config_;
  OnNewRequestCb on_new_request_cb_;
  net::ParserStats& stats_;
  request::ResponseDataAccounter& data_accounter_;

  Http2UpgradeData upgrade_data_;

  HttpProtocol http_protocol_{HttpProtocol::kHTTP1};

  std::optional<HTTP1Handler> http1_handler_;
  std::optional<HttpRequestParser> http1_parser_;

  std::optional<HTTP2Handler> http2_handler_;
  std::optional<Http2RequestParser> http2_parser_;

  bool sendUpgradeData_ = false;
};

}  // namespace server::http

USERVER_NAMESPACE_END
