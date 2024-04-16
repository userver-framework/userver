#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <optional>

#include <llhttp.h>

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
  static int OnMessageBegin(llhttp_t* p);
  static int OnUrl(llhttp_t* p, const char* data, size_t size);
  static int OnHeaderField(llhttp_t* p, const char* data, size_t size);
  static int OnHeaderValue(llhttp_t* p, const char* data, size_t size);
  static int OnHeadersComplete(llhttp_t* p);
  static int OnBody(llhttp_t* p, const char* data, size_t size);
  static int OnMessageComplete(llhttp_t* p);

  int OnMessageBeginImpl(llhttp_t* p);
  int OnUrlImpl(llhttp_t* p, const char* data, size_t size);
  int OnHeaderFieldImpl(llhttp_t* p, const char* data, size_t size);
  int OnHeaderValueImpl(llhttp_t* p, const char* data, size_t size);
  int OnHeadersCompleteImpl(llhttp_t* p);
  int OnBodyImpl(llhttp_t* p, const char* data, size_t size);
  int OnMessageCompleteImpl(llhttp_t* p);

  void CreateRequestConstructor();

  bool CheckUrlComplete(llhttp_t* p);

  bool FinalizeRequest();
  bool FinalizeRequestImpl();

  const HandlerInfoIndex& handler_info_index_;
  const HttpRequestConstructor::Config request_constructor_config_;

  bool url_complete_ = false;

  OnNewRequestCb on_new_request_cb_;

  llhttp_t parser_{};
  std::optional<HttpRequestConstructor> request_constructor_;

  static const llhttp_settings_t parser_settings;
  net::ParserStats& stats_;
  request::ResponseDataAccounter& data_accounter_;
};

}  // namespace server::http

USERVER_NAMESPACE_END
