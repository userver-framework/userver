#include <userver/http/status_code.hpp>

#include <optional>
#include <ostream>

USERVER_NAMESPACE_BEGIN

namespace http {

std::optional<std::string_view> TryStatusCodeString(StatusCode status) {
  switch (status) {
    case StatusCode::kInvalid:
      return "Invalid";
    case StatusCode::kContinue:
      return "Continue";
    case StatusCode::kSwitchingProtocols:
      return "Switching Protocols";
    case StatusCode::kProcessing:
      return "Processing";
    case StatusCode::kEarlyHints:
      return "Early Hints";
    case StatusCode::kOk:
      return "OK";
    case StatusCode::kCreated:
      return "Created";
    case StatusCode::kAccepted:
      return "Accepted";
    case StatusCode::kNonAuthoritativeInformation:
      return "Non-Authoritative Information";
    case StatusCode::kNoContent:
      return "No Content";
    case StatusCode::kResetContent:
      return "Reset Content";
    case StatusCode::kPartialContent:
      return "Partial Content";
    case StatusCode::kMultiStatus:
      return "Multi-Status";
    case StatusCode::kAlreadyReported:
      return "Already Reported";
    case StatusCode::kThisIsFine:
      return "This is Fine";
    case StatusCode::kImUsed:
      return "IM Used";
    case StatusCode::kMultipleChoices:
      return "Multiple Choices";
    case StatusCode::kMovedPermanently:
      return "Moved Permanently";
    case StatusCode::kFound:
      return "Found";
    case StatusCode::kSeeOther:
      return "See Other";
    case StatusCode::kNotModified:
      return "Not Modified";
    case StatusCode::kUseProxy:
      return "Use Proxy";
    case StatusCode::kSwitchProxy:
      return "Switch Proxy";
    case StatusCode::kTemporaryRedirect:
      return "Temporary Redirect";
    case StatusCode::kPermanentRedirect:
      return "Permanent Redirect";
    case StatusCode::kBadRequest:
      return "Bad Request";
    case StatusCode::kUnauthorized:
      return "Unauthorized";
    case StatusCode::kPaymentRequired:
      return "Payment Required";
    case StatusCode::kForbidden:
      return "Forbidden";
    case StatusCode::kNotFound:
      return "Not Found";
    case StatusCode::kMethodNotAllowed:
      return "Method Not Allowed";
    case StatusCode::kNotAcceptable:
      return "Not Acceptable";
    case StatusCode::kProxyAuthenticationRequired:
      return "Proxy Authentication Required";
    case StatusCode::kRequestTimeout:
      return "Request Timeout";
    case StatusCode::kConflict:
      return "Conflict";
    case StatusCode::kGone:
      return "Gone";
    case StatusCode::kLengthRequired:
      return "Length Required";
    case StatusCode::kPreconditionFailed:
      return "Precondition Failed";
    case StatusCode::kPayloadTooLarge:
      return "Payload Too Large";
    case StatusCode::kUriTooLong:
      return "URI Too Long";
    case StatusCode::kUnsupportedMediaType:
      return "Unsupported Media Type";
    case StatusCode::kRangeNotSatisfiable:
      return "Range Not Satisfiable";
    case StatusCode::kExpectationFailed:
      return "Expectation Failed";
    case StatusCode::kImATeapot:
      return "I'm a teapot";
    case StatusCode::kPageExpired:
      return "Page Expired";
    case StatusCode::kMethodFailure:
      return "Method Failure";
    case StatusCode::kMisdirectedRequest:
      return "Misdirected Request";
    case StatusCode::kUnprocessableEntity:
      return "Unprocessable Entity";
    case StatusCode::kLocked:
      return "Locked";
    case StatusCode::kFailedDependency:
      return "Failed Dependency";
    case StatusCode::kTooEarly:
      return "Too Early";
    case StatusCode::kUpgradeRequired:
      return "Upgrade Required";
    case StatusCode::kPreconditionRequired:
      return "Precondition Required";
    case StatusCode::kTooManyRequests:
      return "Too Many Requests";
    case StatusCode::kRequestHeaderFieldsTooLarge:
      return "Request Header Fields Too Large";
    case StatusCode::kNginxNoResponse:
      return "Nginx No Response";
    case StatusCode::kUnavailableForLegalReasons:
      return "Unavailable For Legal Reasons";
    case StatusCode::kNginxRequestHeaderTooLarge:
      return "Nginx Request Header Too Large";
    case StatusCode::kNginxSSLCertificateError:
      return "Nginx SSL Certificate Error";
    case StatusCode::kNginxSSLCertificateRequired:
      return "Nginx SSL Certificate Required";
    case StatusCode::kNginxHTTPRequestSenttoHTTPSPort:
      return "Nginx HTTP Request Sent to HTTPS Port";
    case StatusCode::kDeadlineExpired:
      return "Deadline Expired";
    case StatusCode::kNginxClientClosedRequest:
      return "Nginx Client Closed Request";
    case StatusCode::kInternalServerError:
      return "Internal Server Error";
    case StatusCode::kNotImplemented:
      return "Not Implemented";
    case StatusCode::kBadGateway:
      return "Bad Gateway";
    case StatusCode::kServiceUnavailable:
      return "Service Unavailable";
    case StatusCode::kGatewayTimeout:
      return "Gateway Timeout";
    case StatusCode::kHttpVersionNotSupported:
      return "HTTP Version Not Supported";
    case StatusCode::kVariantAlsoNegotiates:
      return "Variant Also Negotiates";
    case StatusCode::kInsufficientStorage:
      return "Insufficient Storage";
    case StatusCode::kLoopDetected:
      return "Loop Detected";
    case StatusCode::kBandwidthLimitExceeded:
      return "Bandwidth Limit Exceeded";
    case StatusCode::kNotExtended:
      return "Not Extended";
    case StatusCode::kNetworkAuthenticationRequired:
      return "Network Authentication Required";
    case StatusCode::kWebServerIsDown:
      return "Web Server Is Down";
    case StatusCode::kConnectionTimedOut:
      return "Connection Timed Out";
    case StatusCode::kOriginIsUnreachable:
      return "Origin Is Unreachable";
    case StatusCode::kTimeoutOccurred:
      return "Timeout Occurred";
    case StatusCode::kSslHandshakeFailed:
      return "SSL Handshake Failed";
    case StatusCode::kInvalidSslCertificate:
      return "Invalid SSL Certificate";
  }
  return std::nullopt;
}

std::string_view StatusCodeString(StatusCode status) {
  auto string = TryStatusCodeString(status);
  return (string ? *string : "Unknown status");
}

std::string ToString(StatusCode status) {
  return fmt::format("{} {}", status, StatusCodeString(status));
}

std::ostream& operator<<(std::ostream& os, StatusCode s) {
  auto string = TryStatusCodeString(s);
  if (string)
    if (s == StatusCode::kInvalid)
      return os << s << *string;
    else
      return os << *string;
  else
    return os << static_cast<std::underlying_type_t<StatusCode>>(s);
}

}  // namespace http

USERVER_NAMESPACE_END
