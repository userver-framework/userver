#pragma once

#include <atomic>
#include <functional>
#include <memory>

#include <http_parser.h>

#include <server/request/request_config.hpp>
#include <server/request/request_parser.hpp>

#include "http_request_constructor.hpp"

namespace server {
namespace http {

class HttpRequestParser : public request::RequestParser {
 public:
  using OnNewRequestCb =
      std::function<void(std::unique_ptr<request::RequestBase>&&)>;

  HttpRequestParser(const HttpRequestHandler& request_handler,
                    const request::RequestConfig& request_config,
                    OnNewRequestCb&& on_new_request_cb);

  virtual bool Parse(const char* data, size_t size) override;

  virtual size_t ParsingRequestCount() const override;

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

  const HttpRequestHandler& request_handler_;
  HttpRequestConstructor::Config request_constructor_config_;

  bool url_complete_ = false;

  OnNewRequestCb on_new_request_cb_;

  http_parser parser_;
  std::unique_ptr<HttpRequestConstructor> request_constructor_;

  static const http_parser_settings parser_settings;
  std::atomic<size_t> parsing_request_count_{0};
};

}  // namespace http
}  // namespace server
