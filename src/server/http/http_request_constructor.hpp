#pragma once

#include <memory>

#include <http_parser.h>

#include <server/request/request_constructor.hpp>

#include "http_request_handler.hpp"
#include "http_request_impl.hpp"

namespace server {
namespace http {

class HttpRequestConstructor : public request::RequestConstructor {
 public:
  enum class Status {
    kOk,
    kBadRequest,
    kUriTooLong,
    kParseUrlError,
    kHandlerNotFound,
    kHeadersTooLarge,
    kRequestTooLarge,
    kParseArgsError,
    kParseCookiesError,
  };

  struct Config {
    size_t max_url_size = 8192;
    size_t max_request_size = 65536;
    size_t max_headers_size = 65536;
    bool parse_args_from_body = false;
  };

  HttpRequestConstructor(Config config,
                         const HttpRequestHandler& request_handler);

  void SetMethod(http_method method);
  void SetHttpMajor(unsigned short http_major);
  void SetHttpMinor(unsigned short http_minor);

  void AppendUrl(const char* data, size_t size);
  void ParseUrl();
  void AppendHeaderField(const char* data, size_t size);
  void AppendHeaderValue(const char* data, size_t size);
  void AppendBody(const char* data, size_t size);

  virtual std::unique_ptr<request::RequestBase> Finalize() override;

 private:
  void FinalizeImpl();

  static std::string UrlDecode(const char* data, const char* data_end);
  void ParseArgs(const http_parser_url& url);
  void ParseArgs(const char* data, size_t size);
  void AddHeader();
  void ParseCookies();

  void SetStatus(Status status);
  void AccountRequestSize(size_t size);
  void AccountUrlSize(size_t size);
  void AccountHeadersSize(size_t size);

  std::string DumpRequestArgs() const;
  std::string DumpHeaders() const;
  std::string DumpCookies() const;
  void CheckStatus() const;

  Config config_;
  const HttpRequestHandler& request_handler_;

  http_parser_url parsed_url_;
  std::string header_field_;
  std::string header_value_;
  bool header_field_flag_ = false;
  bool header_value_flag_ = false;

  size_t request_size_ = 0;
  size_t url_size_ = 0;
  size_t headers_size_ = 0;
  bool url_parsed_ = false;
  Status status_ = Status::kOk;

  std::unique_ptr<HttpRequestImpl> request_;
};

}  // namespace http
}  // namespace server
