#pragma once

#include <chrono>
#include <string>
#include <unordered_map>

#include <userver/http/content_type.hpp>
#include <userver/server/http/http_response_cookie.hpp>
#include <userver/server/request/response_base.hpp>
#include <userver/utils/projecting_view.hpp>
#include <userver/utils/str_icase.hpp>

#include "http_status.hpp"

USERVER_NAMESPACE_BEGIN

namespace server::http {

namespace impl {

void OutputHeader(std::string& os, std::string_view key, std::string_view val);

}

class HttpRequestImpl;

class HttpResponse final : public request::ResponseBase {
 public:
  using HeadersMap =
      std::unordered_map<std::string, std::string, utils::StrIcaseHash,
                         utils::StrIcaseEqual>;

  using HeadersMapKeys = decltype(utils::MakeKeysView(HeadersMap()));

  using CookiesMap = std::unordered_map<std::string, Cookie>;

  using CookiesMapKeys = decltype(utils::MakeKeysView(CookiesMap()));

  HttpResponse(const HttpRequestImpl& request,
               request::ResponseDataAccounter& data_accounter);
  ~HttpResponse() override;

  void SetSendFailed(
      std::chrono::steady_clock::time_point failure_time) override;

  void SetHeader(std::string name, std::string value);
  void SetContentType(const USERVER_NAMESPACE::http::ContentType& type);
  void SetContentEncoding(std::string encoding);
  void SetStatus(HttpStatus status);
  void ClearHeaders();

  void SetCookie(Cookie cookie);
  void ClearCookies();

  HttpStatus GetStatus() const { return status_; }

  HeadersMapKeys GetHeaderNames() const;
  const std::string& GetHeader(const std::string& header_name) const;
  bool HasHeader(const std::string& header_name) const;

  CookiesMapKeys GetCookieNames() const;
  const Cookie& GetCookie(const std::string& cookie_name) const;

  // TODO: server internals. remove from public interface
  void SendResponse(engine::io::Socket& socket) override;

  void SetStatusServiceUnavailable() override {
    SetStatus(HttpStatus::kServiceUnavailable);
  }
  void SetStatusOk() override { SetStatus(HttpStatus::kOk); }
  void SetStatusNotFound() override { SetStatus(HttpStatus::kNotFound); }

 private:
  const HttpRequestImpl& request_;
  HttpStatus status_ = HttpStatus::kOk;
  HeadersMap headers_;
  CookiesMap cookies_;
};

}  // namespace server::http

USERVER_NAMESPACE_END
