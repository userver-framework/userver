#pragma once

#include <memory>

#include <http_parser.h>

#include <server/request/request_constructor.hpp>

#include "http_request_impl.hpp"

namespace server {
namespace http {

class HttpRequestConstructor : public request::RequestConstructor {
 public:
  HttpRequestConstructor();
  virtual ~HttpRequestConstructor();

  void SetMethod(http_method method);
  void SetHttpMajor(unsigned short http_major);
  void SetHttpMinor(unsigned short http_minor);
  void SetParseArgsFromBody(bool parse_args_from_body);

  void AppendUrl(const char* data, size_t size);
  void AppendHeaderField(const char* data, size_t size);
  void AppendHeaderValue(const char* data, size_t size);
  void AppendBody(const char* data, size_t size);

  virtual std::unique_ptr<request::RequestBase> Finalize() override;

 private:
  static std::string UrlDecode(const char* data, const char* data_end);
  void ParseArgs(const http_parser_url& url);
  void ParseArgs(const char* data, size_t size);
  void AddHeader();
  void ParseCookies();

  std::unique_ptr<HttpRequestImpl> request_;

  std::string header_field_;
  std::string header_value_;
  bool header_field_flag_ = false;
  bool header_value_flag_ = false;

  bool parse_args_from_body_ = false;
};

}  // namespace http
}  // namespace server
