#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <server/http/http_method.hpp>
#include <server/http/http_response.hpp>
#include <server/http/http_types.hpp>
#include <server/request/request_base.hpp>

namespace server {
namespace http {

class HttpRequestImpl : public request::RequestBase {
 public:
  HttpRequestImpl();
  virtual ~HttpRequestImpl();

  const HttpMethod& GetMethod() const { return method_; }
  const HttpMethod& GetOrigMethod() const { return orig_method_; }
  const std::string& GetMethodStr() const { return ToString(method_); }
  const std::string& GetOrigMethodStr() const { return ToString(orig_method_); }
  int GetHttpMajor() const { return http_major_; }
  int GetHttpMinor() const { return http_minor_; }
  const std::string& GetUrl() const { return url_; }
  const std::string& GetRequestPath() const override { return request_path_; }
  const std::string& GetPathSuffix() const { return path_suffix_; }
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

  const std::string& RequestBody() const { return request_body_; }
  void SetResponseStatus(HttpStatus status) const {
    response_->SetStatus(status);
  }

  virtual request::ResponseBase& GetResponse() const override {
    return *response_;
  }
  HttpResponse& GetHttpResponse() const { return *response_; }

  virtual void WriteAccessLogs(
      const logging::LoggerPtr& logger_access,
      const logging::LoggerPtr& logger_access_tskv,
      const std::string& remote_address) const override;

  void WriteAccessLog(const logging::LoggerPtr& logger_access,
                      const std::string& remote_address) const;

  void WriteAccessTskvLog(const logging::LoggerPtr& logger_access_tskv,
                          const std::string& remote_address) const;

  virtual void SetMatchedPathLength(size_t length) override;

  friend class HttpRequestConstructor;

 private:
  // method_ = (orig_method_ == kHead ? kGet : orig_method_)
  HttpMethod method_;
  HttpMethod orig_method_;
  unsigned short http_major_;
  unsigned short http_minor_;
  std::string url_;
  std::string request_path_;
  std::string request_body_;
  std::string path_suffix_;
  std::unordered_map<std::string, std::vector<std::string>> request_args_;
  HeadersMap headers_;
  CookiesMap cookies_;

  std::unique_ptr<HttpResponse> response_;
};

}  // namespace http
}  // namespace server
