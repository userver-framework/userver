#pragma once

/// @file userver/server/http/http_response.hpp
/// @brief @copybrief server::http::HttpResponse

#include <chrono>
#include <string>
#include <unordered_map>

#include <userver/engine/single_consumer_event.hpp>
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

/// @brief HTTP Response data
class HttpResponse final : public request::ResponseBase {
 public:
  using HeadersMap =
      std::unordered_map<std::string, std::string, utils::StrIcaseHash,
                         utils::StrIcaseEqual>;

  using HeadersMapKeys = decltype(utils::MakeKeysView(HeadersMap()));

  using CookiesMap = std::unordered_map<std::string_view, Cookie>;

  using CookiesMapKeys = decltype(utils::MakeKeysView(CookiesMap()));

  /// @cond
  HttpResponse(const HttpRequestImpl& request,
               request::ResponseDataAccounter& data_accounter);
  ~HttpResponse() override;

  void SetSendFailed(
      std::chrono::steady_clock::time_point failure_time) override;
  /// @endcond

  /// @brief Add a new response header or rewrite an existing one.
  void SetHeader(std::string name, std::string value);

  /// @brief Add or rewrite the Content-Type header.
  void SetContentType(const USERVER_NAMESPACE::http::ContentType& type);

  /// @brief Add or rewrite the Content-Encoding header.
  void SetContentEncoding(std::string encoding);

  /// @brief Set the HTTP response status code.
  void SetStatus(HttpStatus status);

  /// @brief Remove all headers from response.
  void ClearHeaders();

  /// @brief Sets a cookie if it was not set before.
  void SetCookie(Cookie cookie);

  /// @brief Remove all cookies from response.
  void ClearCookies();

  /// @return HTTP response status
  HttpStatus GetStatus() const { return status_; }

  /// @return List of HTTP headers names.
  HeadersMapKeys GetHeaderNames() const;

  /// @return Value of the header with case insensitive name header_name, or an
  /// empty string if no such header.
  const std::string& GetHeader(const std::string& header_name) const;

  /// @return true if header with case insensitive name header_name exists,
  /// false otherwise.
  bool HasHeader(const std::string& header_name) const;

  /// @return List of cookies names.
  CookiesMapKeys GetCookieNames() const;

  /// @return Value of the cookie with case sensitive name cookie_name, or an
  /// empty string if no such cookie exists.
  const Cookie& GetCookie(std::string_view cookie_name) const;

  /// @cond
  // TODO: server internals. remove from public interface
  void SendResponse(engine::io::Socket& socket) override;
  /// @endcond

  void SetStatusServiceUnavailable() override {
    SetStatus(HttpStatus::kServiceUnavailable);
  }
  void SetStatusOk() override { SetStatus(HttpStatus::kOk); }
  void SetStatusNotFound() override { SetStatus(HttpStatus::kNotFound); }

  bool WaitForHeadersEnd() override;
  void SetHeadersEnd() override;

  void SetStreamBody();
  bool IsBodyStreamed() const override;
  // Can be called only once
  Queue::Producer GetBodyProducer();

 private:
  void SetBodyStreamed(engine::io::Socket& socket, std::string& os);
  void SetBodyNotstreamed(engine::io::Socket& socket, std::string& os);

  const HttpRequestImpl& request_;
  HttpStatus status_ = HttpStatus::kOk;
  HeadersMap headers_;
  CookiesMap cookies_;

  engine::SingleConsumerEvent headers_end_;
  std::shared_ptr<Queue> body_queue_;
  std::optional<QueueConsumer> body_stream_;
  std::optional<QueueProducer> body_stream_producer_;
};

void SetThrottleReason(http::HttpResponse& http_response,
                       std::string log_reason, std::string http_header_reason);

}  // namespace server::http

USERVER_NAMESPACE_END
