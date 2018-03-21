#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <http_parser.h>

#include <server/request/request_base.hpp>
#include <utils/str_icase.hpp>

#include "http_response.hpp"

namespace server {
namespace http {

class HttpRequestImpl : public request::RequestBase {
 public:
  HttpRequestImpl();
  virtual ~HttpRequestImpl();

  const http_method& GetMethod() const { return method_; }
  std::string GetMethodStr() const { return http_method_str(method_); }
  int GetHttpMajor() const { return http_major_; }
  int GetHttpMinor() const { return http_minor_; }
  const std::string& GetUrl() const { return url_; }
  const std::string& GetRequestPath() const { return request_path_; }
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
  std::vector<std::string> HeaderNames() const;

  const std::string& GetCookie(const std::string& cookie_name) const;
  bool HasCookie(const std::string& cookie_name) const;
  size_t CookieCount() const;
  std::vector<std::string> CookieNames() const;

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
      const std::string& remote_host,
      const std::string& remote_address) const override;

  void WriteAccessLog(const logging::LoggerPtr& logger_access,
                      const std::string& remote_host,
                      const std::string& remote_address) const;

  void WriteAccessTskvLog(const logging::LoggerPtr& logger_access_tskv,
                          const std::string& remote_host,
                          const std::string& remote_address) const;

  virtual void SetMatchedPathLength(size_t length) override;

  friend class HttpRequestConstructor;

 private:
  http_method method_;
  unsigned short http_major_;
  unsigned short http_minor_;
  std::string url_;
  std::string request_path_;
  std::string request_body_;
  std::string path_suffix_;
  std::unordered_map<std::string, std::vector<std::string>> request_args_;
  std::unordered_map<std::string, std::string, utils::StrIcaseHash,
                     utils::StrIcaseCmp>
      headers_;
  std::unordered_map<std::string, std::string, utils::StrIcaseHash,
                     utils::StrIcaseCmp>
      cookies_;

  std::unique_ptr<HttpResponse> response_;
};

}  // namespace http
}  // namespace server
