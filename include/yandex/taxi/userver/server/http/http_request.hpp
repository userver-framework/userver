#pragma once

#include <chrono>
#include <string>
#include <vector>

#include <http_parser.h>

#include <yandex/taxi/userver/server/http/http_response.hpp>
#include <yandex/taxi/userver/server/http/http_types.hpp>

namespace server {
namespace http {

class HttpRequestImpl;

class HttpRequest {
 public:
  explicit HttpRequest(const HttpRequestImpl& impl);
  ~HttpRequest() = default;

  request::ResponseBase& GetResponse() const;
  HttpResponse& GetHttpResponse() const;

  const http_method& GetMethod() const;
  std::string GetMethodStr() const;
  int GetHttpMajor() const;
  int GetHttpMinor() const;
  const std::string& GetUrl() const;
  const std::string& GetRequestPath() const;
  const std::string& GetPathSuffix() const;
  std::chrono::duration<double> GetRequestTime() const;
  std::chrono::duration<double> GetResponseTime() const;

  const std::string& GetHost() const;

  const std::string& GetArg(const std::string& arg_name) const;
  const std::vector<std::string>& GetArgVector(
      const std::string& arg_name) const;
  bool HasArg(const std::string& arg_name) const;
  size_t ArgCount() const;
  std::vector<std::string> ArgNames() const;

  const std::string& GetHeader(const std::string& header_name) const;
  bool HasHeader(const std::string& header_name) const;
  size_t HeaderCount() const;
  HeadersMapKeys GetHeaderNames() const;

  const std::string& GetCookie(const std::string& cookie_name) const;
  bool HasCookie(const std::string& cookie_name) const;
  size_t CookieCount() const;
  CookiesMapKeys GetCookieNames() const;

  const std::string& RequestBody() const;
  void SetResponseStatus(HttpStatus status) const;

 private:
  const HttpRequestImpl& impl_;
};

}  // namespace http
}  // namespace server
