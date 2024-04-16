#pragma once

/// @file userver/clients/http/response.hpp
/// @brief @copybrief clients::http::Response

#include <iosfwd>
#include <string>

#include <userver/clients/http/error.hpp>
#include <userver/clients/http/local_stats.hpp>
#include <userver/http/header_map.hpp>
#include <userver/server/http/http_response_cookie.hpp>
#include <userver/utils/str_icase.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

/// https://en.wikipedia.org/wiki/List_of_HTTP_status_codes
enum Status : uint16_t {
  Invalid = 0,

  // 1xx informational response
  Continue = 100,
  SwitchingProtocols = 101,
  Processing = 102,
  EarlyHints = 103,

  // 2xx success
  OK = 200,
  Created = 201,
  Accepted = 202,
  NonAuthoritativeInformation = 203,
  NoContent = 204,
  ResetContent = 205,
  PartialContent = 206,
  MultiStatus = 207,
  AlreadyReported = 208,
  ThisIsFine = 218,
  IMUsed = 226,

  // 3xx redirection
  MultipleChoices = 300,
  MovedPermanently = 301,
  Found = 302,
  SeeOther = 303,
  NotModified = 304,
  UseProxy = 305,
  SwitchProxy = 306,
  TemporaryRedirect = 307,
  PermanentRedirect = 308,

  // 4xx client errors
  BadRequest = 400,
  Unauthorized = 401,
  PaymentRequired = 402,
  Forbidden = 403,
  NotFound = 404,
  MethodNotAllowed = 405,
  NotAcceptable = 406,
  ProxyAuthenticationRequired = 407,
  RequestTimeout = 408,
  Conflict = 409,
  Gone = 410,
  LengthRequired = 411,
  PreconditionFailed = 412,
  PayloadTooLarge = 413,
  UriTooLong = 414,
  UnsupportedMediaType = 415,
  RangeNotSatisfiable = 416,
  ExpectationFailed = 417,
  ImATeapot = 418,
  PageExpired = 419,
  MethodFailure = 420,
  MisdirectedRequest = 421,
  UnprocessableEntity = 422,
  Locked = 423,
  FailedDependency = 424,
  TooEarly = 425,
  UpgradeRequired = 426,
  PreconditionRequired = 428,
  TooManyRequests = 429,
  RequestHeaderFieldsTooLarge = 431,
  NginxNoResponse = 444,
  UnavailableForLegalReasons = 451,
  NginxRequestHeaderTooLarge = 494,
  NginxSSLCertificateError = 495,
  NginxSSLCertificateRequired = 496,
  NginxHTTPRequestSenttoHTTPSPort = 497,
  NginxClientClosedRequest = 499,

  // 5xx server errors
  InternalServerError = 500,
  NotImplemented = 501,
  BadGateway = 502,
  ServiceUnavailable = 503,
  GatewayTimeout = 504,
  HttpVersionNotSupported = 505,
  VariantAlsoNegotiates = 506,
  InsufficientStorage = 507,
  LoopDetected = 508,
  BandwidthLimitExceeded = 509,
  NotExtended = 510,
  NetworkAuthenticationRequired = 511,
  WebServerIsDown = 520,
  ConnectionTimedOut = 522,
  OriginIsUnreachable = 523,
  TimeoutOccurred = 524,
  SslHandshakeFailed = 525,
  InvalidSslCertificate = 526,
};

std::ostream& operator<<(std::ostream& os, Status s);

/// Headers container type
using Headers = USERVER_NAMESPACE::http::headers::HeaderMap;

/// Class that will be returned for successful request
class Response final {
 public:
  using CookiesMap = server::http::Cookie::CookiesMap;

  Response() = default;

  /// response string
  std::string& sink_string() { return response_; }

  /// body as string
  std::string body() const& { return response_; }
  std::string&& body() && { return std::move(response_); }

  /// body as string_view
  std::string_view body_view() const { return response_; }

  /// return reference to headers
  const Headers& headers() const { return headers_; }
  Headers& headers() { return headers_; }
  const CookiesMap& cookies() const { return cookies_; }
  CookiesMap& cookies() { return cookies_; }

  /// status_code
  Status status_code() const;
  /// check status code
  bool IsOk() const { return status_code() == Status::OK; }
  bool IsError() const { return static_cast<uint16_t>(status_code()) >= 400; }

  static void RaiseForStatus(int code, const LocalStats& stats);

  void raise_for_status() const;

  /// returns statistics on request execution like count of opened sockets,
  /// connect time...
  LocalStats GetStats() const;

  void SetStats(const LocalStats& stats) { stats_ = stats; }
  void SetStatusCode(Status status_code) { status_code_ = status_code; }

 private:
  Headers headers_;
  CookiesMap cookies_;
  std::string response_;
  Status status_code_{Status::Invalid};
  LocalStats stats_;
};

}  // namespace clients::http

USERVER_NAMESPACE_END
