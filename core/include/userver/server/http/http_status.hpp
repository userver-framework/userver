#pragma once

/// @file userver/server/http/http_status.hpp
/// @brief @copybrief server::http::HttpStatus

#include <string>

#include <fmt/core.h>
#include <userver/utils/fmt_compat.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

/// @brief HTTP status codes
enum class HttpStatus {
  kContinue = 100,
  kSwitchingProtocols = 101,
  kProcessing = 102,
  kOk = 200,
  kCreated = 201,
  kAccepted = 202,
  kNonAuthoritativeInformation = 203,
  kNoContent = 204,
  kResetContent = 205,
  kPartialContent = 206,
  kMultiStatus = 207,
  kAlreadyReported = 208,
  kImUsed = 226,
  kMultipleChoices = 300,
  kMovedPermanently = 301,
  kFound = 302,
  kSeeOther = 303,
  kNotModified = 304,
  kUseProxy = 305,
  kTemporaryRedirect = 307,
  kPermanentRedirect = 308,
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
  kMisdirectedRequest = 421,
  kUnprocessableEntity = 422,
  kLocked = 423,
  kFailedDependency = 424,
  kUpgradeRequired = 426,
  kPreconditionRequired = 428,
  kTooManyRequests = 429,
  kRequestHeaderFieldsTooLarge = 431,
  kUnavailableForLegalReasons = 451,
  kClientClosedRequest = 499,
  kInternalServerError = 500,
  kNotImplemented = 501,
  kBadGateway = 502,
  kServiceUnavailable = 503,
  kGatewayTimeout = 504,
  kHttpVersionNotSupported = 505,
  kVariantAlsoNegotiates = 506,
  kInsufficientStorage = 507,
  kLoopDetected = 508,
  kNotExtended = 510,
  kNetworkAuthenticationRequired = 511,
};

std::string HttpStatusString(HttpStatus status);

inline std::string ToString(HttpStatus status) {
  return server::http::HttpStatusString(status);
}

}  // namespace server::http

USERVER_NAMESPACE_END

template <>
struct fmt::formatter<USERVER_NAMESPACE::server::http::HttpStatus> {
  static auto parse(format_parse_context& ctx) { return ctx.begin(); }

  template <typename FormatContext>
  auto format(USERVER_NAMESPACE::server::http::HttpStatus status,
              FormatContext& ctx) USERVER_FMT_CONST {
    return fmt::format_to(
        ctx.out(), "{}",
        USERVER_NAMESPACE::server::http::HttpStatusString(status));
  }
};
