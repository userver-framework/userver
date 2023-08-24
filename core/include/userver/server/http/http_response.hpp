#pragma once

/// @file userver/server/http/http_response.hpp
/// @brief @copybrief server::http::HttpResponse

#include <chrono>
#include <string>
#include <unordered_map>

#include <userver/concurrent/queue.hpp>
#include <userver/engine/single_consumer_event.hpp>
#include <userver/http/content_type.hpp>
#include <userver/http/header_map.hpp>
#include <userver/server/http/http_response_cookie.hpp>
#include <userver/server/request/response_base.hpp>
#include <userver/utils/impl/projecting_view.hpp>
#include <userver/utils/str_icase.hpp>

#include "http_status.hpp"

USERVER_NAMESPACE_BEGIN

namespace server::http {

namespace impl {

void OutputHeader(std::string& header, std::string_view key,
                  std::string_view val);

}

class HttpRequestImpl;

/// @brief HTTP Response data
class HttpResponse final : public request::ResponseBase {
 public:
  using HeadersMap = USERVER_NAMESPACE::http::headers::HeaderMap;

  using HeadersMapKeys = decltype(utils::impl::MakeKeysView(HeadersMap()));

  using CookiesMap = Cookie::CookiesMap;

  using CookiesMapKeys = decltype(utils::impl::MakeKeysView(CookiesMap()));

  /// @cond
  HttpResponse(const HttpRequestImpl& request,
               request::ResponseDataAccounter& data_accounter);
  ~HttpResponse() override;

  void SetSendFailed(
      std::chrono::steady_clock::time_point failure_time) override;
  /// @endcond

  /// @brief Add a new response header or rewrite an existing one.
  /// @returns true if the header was set. Returns false if headers
  /// were already sent for stream'ed response and the new header was not set.
  bool SetHeader(std::string name, std::string value);

  /// @brief Add a new response header or rewrite an existing one.
  /// @returns true if the header was set. Returns false if headers
  /// were already sent for stream'ed response and the new header was not set.
  bool SetHeader(std::string_view name, std::string value);

  /// @overload
  bool SetHeader(
      const USERVER_NAMESPACE::http::headers::PredefinedHeader& header,
      std::string value);

  /// @brief Add or rewrite the Content-Type header.
  void SetContentType(const USERVER_NAMESPACE::http::ContentType& type);

  /// @brief Add or rewrite the Content-Encoding header.
  void SetContentEncoding(std::string encoding);

  /// @brief Set the HTTP response status code.
  /// @returns true if the status was set. Returns false if headers
  /// were already sent for stream'ed response and the new status was not set.
  bool SetStatus(HttpStatus status);

  /// @brief Remove all headers from response.
  /// @returns true if the headers were cleared. Returns false if headers
  /// were already sent for stream'ed response and the headers were not cleared.
  bool ClearHeaders();

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
  const std::string& GetHeader(std::string_view header_name) const;
  /// @overload
  const std::string& GetHeader(
      const USERVER_NAMESPACE::http::headers::PredefinedHeader& header_name)
      const;

  /// @return true if header with case insensitive name header_name exists,
  /// false otherwise.
  bool HasHeader(std::string_view header_name) const;
  /// @overload
  bool HasHeader(const USERVER_NAMESPACE::http::headers::PredefinedHeader&
                     header_name) const;

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

  using Queue = concurrent::StringStreamQueue;

  void SetStreamBody();
  bool IsBodyStreamed() const override;
  // Can be called only once
  Queue::Producer GetBodyProducer();

 private:
  // Returns total size of the response
  std::size_t SetBodyStreamed(engine::io::Socket& socket, std::string& header);

  // Returns total size of the response
  std::size_t SetBodyNotStreamed(engine::io::Socket& socket,
                                 std::string& header);

  const HttpRequestImpl& request_;
  HttpStatus status_ = HttpStatus::kOk;
  HeadersMap headers_;
  CookiesMap cookies_;

  engine::SingleConsumerEvent headers_end_;
  std::optional<Queue::Consumer> body_stream_;
  std::optional<Queue::Producer> body_stream_producer_;
};

void SetThrottleReason(http::HttpResponse& http_response,
                       std::string log_reason, std::string http_header_reason);

}  // namespace server::http

USERVER_NAMESPACE_END
