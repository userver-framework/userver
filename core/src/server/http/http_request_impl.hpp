#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <userver/engine/task/task_processor_fwd.hpp>

#include <userver/server/http/http_method.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_response.hpp>
#include <userver/server/request/request_base.hpp>
#include <userver/utils/datetime/wall_coarse_clock.hpp>
#include <userver/utils/str_icase.hpp>

USERVER_NAMESPACE_BEGIN

namespace server {
namespace handlers {
class HttpRequestStatistics;
class HttpHandlerBase;
}  // namespace handlers

namespace http {

class HttpRequestImpl final : public request::RequestBase {
 public:
  HttpRequestImpl(request::ResponseDataAccounter& data_accounter);
  ~HttpRequestImpl() override;

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

  const FormDataArg& GetFormDataArg(const std::string& arg_name) const;
  const std::vector<FormDataArg>& GetFormDataArgVector(
      const std::string& arg_name) const;
  bool HasFormDataArg(const std::string& arg_name) const;
  size_t FormDataArgCount() const;
  std::vector<std::string> FormDataArgNames() const;

  const std::string& GetPathArg(const std::string& arg_name) const;
  const std::string& GetPathArg(size_t index) const;
  bool HasPathArg(const std::string& arg_name) const;
  bool HasPathArg(size_t index) const;
  size_t PathArgCount() const;

  const std::string& GetHeader(std::string_view header_name) const;
  const std::string& GetHeader(
      const USERVER_NAMESPACE::http::headers::PredefinedHeader& header_name)
      const;
  bool HasHeader(std::string_view header_name) const;
  bool HasHeader(const USERVER_NAMESPACE::http::headers::PredefinedHeader&
                     header_name) const;
  size_t HeaderCount() const;
  void RemoveHeader(std::string_view header_name);
  void RemoveHeader(
      const USERVER_NAMESPACE::http::headers::PredefinedHeader& header_name);
  HttpRequest::HeadersMapKeys GetHeaderNames() const;
  const HttpRequest::HeadersMap& GetHeaders() const;

  const std::string& GetCookie(const std::string& cookie_name) const;
  bool HasCookie(const std::string& cookie_name) const;
  size_t CookieCount() const;
  HttpRequest::CookiesMapKeys GetCookieNames() const;
  const HttpRequest::CookiesMap& GetCookies() const;

  const std::string& RequestBody() const { return request_body_; }
  void SetRequestBody(std::string body);
  void ParseArgsFromBody();
  void SetResponseStatus(HttpStatus status) const {
    response_.SetStatus(status);
  }

  bool IsBodyCompressed() const;

  bool IsFinal() const override { return is_final_; }

  request::ResponseBase& GetResponse() const override { return response_; }
  HttpResponse& GetHttpResponse() const { return response_; }

  void WriteAccessLogs(const logging::LoggerPtr& logger_access,
                       const logging::LoggerPtr& logger_access_tskv,
                       const std::string& remote_address) const override;

  void WriteAccessLog(const logging::LoggerPtr& logger_access,
                      utils::datetime::WallCoarseClock::time_point tp,
                      const std::string& remote_address) const;

  void WriteAccessTskvLog(const logging::LoggerPtr& logger_access_tskv,
                          utils::datetime::WallCoarseClock::time_point tp,
                          const std::string& remote_address) const;

  void SetPathArgs(std::vector<std::pair<std::string, std::string>> args);

  void SetMatchedPathLength(size_t length) override;

  void AccountResponseTime() override;

  void MarkAsInternalServerError() const override;

  void SetHttpHandler(const handlers::HttpHandlerBase& handler);
  const handlers::HttpHandlerBase* GetHttpHandler() const;

  void SetTaskProcessor(engine::TaskProcessor& task_processor);
  engine::TaskProcessor* GetTaskProcessor() const;

  void SetHttpHandlerStatistics(handlers::HttpRequestStatistics&);

  friend class HttpRequestConstructor;

 private:
  // method_ = (orig_method_ == kHead ? kGet : orig_method_)
  HttpMethod method_{HttpMethod::kUnknown};
  HttpMethod orig_method_{HttpMethod::kUnknown};
  unsigned short http_major_{1};
  unsigned short http_minor_{1};
  std::string url_;
  std::string request_path_;
  std::string request_body_;
  std::string path_suffix_;
  std::unordered_map<std::string, std::vector<std::string>, utils::StrCaseHash>
      request_args_;
  std::unordered_map<std::string, std::vector<FormDataArg>, utils::StrCaseHash>
      form_data_args_;
  std::vector<std::string> path_args_;
  std::unordered_map<std::string, size_t, utils::StrCaseHash>
      path_args_by_name_index_;
  HttpRequest::HeadersMap headers_;
  HttpRequest::CookiesMap cookies_;
  bool is_final_{false};

  mutable HttpResponse response_;
  engine::TaskProcessor* task_processor_{nullptr};
  const handlers::HttpHandlerBase* handler_{nullptr};
  handlers::HttpRequestStatistics* request_statistics_{nullptr};
};

}  // namespace http
}  // namespace server

USERVER_NAMESPACE_END
