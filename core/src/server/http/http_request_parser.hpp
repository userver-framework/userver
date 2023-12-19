#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <optional>

#include <http_parser.h>

#include <server/net/stats.hpp>
#include <server/request/request_parser.hpp>

#include <userver/server/request/request_config.hpp>

#include "http_request_constructor.hpp"

USERVER_NAMESPACE_BEGIN

namespace server::http {

class HttpRequestParser final : public request::RequestParser {
 public:
  using OnNewRequestCb =
      std::function<void(std::shared_ptr<request::RequestBase>&&)>;

  HttpRequestParser(const HandlerInfoIndex& handler_info_index,
                    const request::HttpRequestConfig& request_config,
                    OnNewRequestCb&& on_new_request_cb, net::ParserStats& stats,
                    request::ResponseDataAccounter& data_accounter);

  HttpRequestParser(HttpRequestParser&&) = delete;
  HttpRequestParser& operator=(HttpRequestParser&&) = delete;

  bool Parse(const char* data, size_t size) override;

 private:
  static int OnMessageBegin(http_parser* p);
  static int OnUrl(http_parser* p, const char* data, size_t size);
  static int OnHeaderField(http_parser* p, const char* data, size_t size);
  static int OnHeaderValue(http_parser* p, const char* data, size_t size);
  static int OnHeadersComplete(http_parser* p);
  static int OnBody(http_parser* p, const char* data, size_t size);
  static int OnMessageComplete(http_parser* p);

  int OnMessageBeginImpl(http_parser* p);
  int OnUrlImpl(http_parser* p, const char* data, size_t size);
  int OnHeaderFieldImpl(http_parser* p, const char* data, size_t size);
  int OnHeaderValueImpl(http_parser* p, const char* data, size_t size);
  int OnHeadersCompleteImpl(http_parser* p);
  int OnBodyImpl(http_parser* p, const char* data, size_t size);
  int OnMessageCompleteImpl(http_parser* p);

  void CreateRequestConstructor();

  bool CheckUrlComplete(http_parser* p);

  bool FinalizeRequest();
  bool FinalizeRequestImpl();

  const HandlerInfoIndex& handler_info_index_;
  const HttpRequestConstructor::Config request_constructor_config_;

  bool url_complete_ = false;

  OnNewRequestCb on_new_request_cb_;

  http_parser parser_{};
  std::optional<HttpRequestConstructor> request_constructor_;

  static const http_parser_settings parser_settings;
  net::ParserStats& stats_;
  request::ResponseDataAccounter& data_accounter_;
};

}  // namespace server::http

USERVER_NAMESPACE_END
