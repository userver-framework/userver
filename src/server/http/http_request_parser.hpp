#pragma once

#include <atomic>
#include <functional>
#include <memory>

#include <http_parser.h>

#include <server/request/request_parser.hpp>

#include "http_request_constructor.hpp"

namespace server {
namespace http {

class HttpRequestParser : public request::RequestParser {
 public:
  using OnNewRequestCb =
      std::function<void(std::unique_ptr<request::RequestBase>&&)>;

  explicit HttpRequestParser(OnNewRequestCb&& on_new_request_cb);
  virtual ~HttpRequestParser();

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

  bool UpdateRequestSize(size_t size);

  size_t max_request_size_ = 65536;

  size_t request_size_ = 0;

  OnNewRequestCb on_new_request_cb_;

  http_parser parser_;
  std::unique_ptr<HttpRequestConstructor> request_constructor_;

  static const http_parser_settings parser_settings;
  std::atomic<size_t> parsing_request_count_{0};
};

}  // namespace http
}  // namespace server
