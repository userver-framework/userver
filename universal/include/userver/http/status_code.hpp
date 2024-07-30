#pragma once

/// @file userver/http/status_code.hpp
/// @brief @copybrief http::StatusCode

#include <cstdint>
#include <iosfwd>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace http {

/// https://en.wikipedia.org/wiki/List_of_HTTP_status_codes
enum StatusCode : uint16_t {
  kInvalid = 0,

  // 1xx informational response
  kContinue = 100,
  kSwitchingProtocols = 101,
  kProcessing = 102,
  kEarlyHints = 103,

  // 2xx success
  kOk = 200,
  kCreated = 201,
  kAccepted = 202,
  kNonAuthoritativeInformation = 203,
  kNoContent = 204,
  kResetContent = 205,
  kPartialContent = 206,
  kMultiStatus = 207,
  kAlreadyReported = 208,
  kThisIsFine = 218,
  kImUsed = 226,

  // 3xx redirection
  kMultipleChoices = 300,
  kMovedPermanently = 301,
  kFound = 302,
  kSeeOther = 303,
  kNotModified = 304,
  kUseProxy = 305,
  kSwitchProxy = 306,
  kTemporaryRedirect = 307,
  kPermanentRedirect = 308,

  // 4xx client errors
  kBadRequest = 400,
  kUnauthorized = 401,
  kPaymentRequired = 402,
  kForbidden = 403,
  kNotFound = 404,
  kMethodNotAllowed = 405,
  kNotAcceptable = 406,
  kProxyAuthenticationRequired = 407,
  kRequestTimeout = 408,
  kConflict = 409,
  kGone = 410,
  kLengthRequired = 411,
  kPreconditionFailed = 412,
  kPayloadTooLarge = 413,
  kUriTooLong = 414,
  kUnsupportedMediaType = 415,
  kRangeNotSatisfiable = 416,
  kExpectationFailed = 417,
  kImATeapot = 418,
  kPageExpired = 419,
  kMethodFailure = 420,
  kMisdirectedRequest = 421,
  kUnprocessableEntity = 422,
  kLocked = 423,
  kFailedDependency = 424,
  kTooEarly = 425,
  kUpgradeRequired = 426,
  kPreconditionRequired = 428,
  kTooManyRequests = 429,
  kRequestHeaderFieldsTooLarge = 431,
  kNginxNoResponse = 444,
  kUnavailableForLegalReasons = 451,
  kNginxRequestHeaderTooLarge = 494,
  kNginxSSLCertificateError = 495,
  kNginxSSLCertificateRequired = 496,
  kNginxHTTPRequestSenttoHTTPSPort = 497,
  kDeadlineExpired = 498,  // userver-specific
  kNginxClientClosedRequest = 499,

  // 5xx server errors
  kInternalServerError = 500,
  kNotImplemented = 501,
  kBadGateway = 502,
  kServiceUnavailable = 503,
  kGatewayTimeout = 504,
  kHttpVersionNotSupported = 505,
  kVariantAlsoNegotiates = 506,
  kInsufficientStorage = 507,
  kLoopDetected = 508,
  kBandwidthLimitExceeded = 509,
  kNotExtended = 510,
  kNetworkAuthenticationRequired = 511,
  kWebServerIsDown = 520,
  kConnectionTimedOut = 522,
  kOriginIsUnreachable = 523,
  kTimeoutOccurred = 524,
  kSslHandshakeFailed = 525,
  kInvalidSslCertificate = 526,

  // migration aliases
  Invalid = kInvalid,
  OK = kOk,
  Created = kCreated,
  NoContent = kNoContent,
  BadRequest = kBadRequest,
  NotFound = kNotFound,
  Conflict = kConflict,
  TooManyRequests = kTooManyRequests,
  InternalServerError = kInternalServerError,
  kClientClosedRequest = kNginxClientClosedRequest,
};

std::string_view StatusCodeString(StatusCode status);

std::string ToString(StatusCode status);

std::ostream& operator<<(std::ostream& os, StatusCode s);

}  // namespace http

USERVER_NAMESPACE_END

template <>
struct fmt::formatter<USERVER_NAMESPACE::http::StatusCode> {
  constexpr static auto parse(format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(USERVER_NAMESPACE::http::StatusCode status,
              FormatContext& ctx) const {
    return fmt::format_to(ctx.out(), "{} {}", static_cast<int>(status),
                          USERVER_NAMESPACE::http::StatusCodeString(status));
  }
};
