#pragma once

#include <chrono>
#include <string>
#include <unordered_map>

#include <boost/range/adaptor/map.hpp>

#include <server/http/http_response_cookie.hpp>
#include <server/request/response_base.hpp>
#include <utils/str_icase.hpp>

#include "http_status.hpp"

namespace server {
namespace http {

class HttpRequestImpl;

class HttpResponse final : public request::ResponseBase {
 public:
  using HeadersMap =
      std::unordered_map<std::string, std::string, utils::StrIcaseHash,
                         utils::StrIcaseCmp>;

  using HeadersMapKeys = decltype(HeadersMap() | boost::adaptors::map_keys);

  using CookiesMap =
      std::unordered_map<std::string, Cookie, utils::StrIcaseHash,
                         utils::StrIcaseCmp>;

  using CookiesMapKeys = decltype(CookiesMap() | boost::adaptors::map_keys);

  explicit HttpResponse(const HttpRequestImpl& request);
  ~HttpResponse() override;

  void SetSendFailed(
      std::chrono::steady_clock::time_point failure_time) override;

  void SetHeader(std::string name, std::string value);
  void SetContentType(std::string type);
  void SetContentEncoding(std::string encoding);
  void SetStatus(HttpStatus status);
  void ClearHeaders();

  void SetCookie(Cookie cookie);
  void ClearCookies();

  HttpStatus GetStatus() const { return status_; }

  HeadersMapKeys GetHeaderNames() const;
  const std::string& GetHeader(const std::string& header_name) const;

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

}  // namespace http
}  // namespace server
