#include <userver/http/status_code.hpp>

#include <ostream>


USERVER_NAMESPACE_BEGIN

namespace http {

std::string_view StatusCodeString(StatusCode status) {
  switch (status) {
    case StatusCode::kContinue:
      return "Continue";
    case StatusCode::kSwitchingProtocols:
      return "Switching Protocols";
    case StatusCode::kProcessing:
      return "Processing";
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
    case StatusCode::kUnavailableForLegalReasons:
      return "Unavailable For Legal Reasons";
    case StatusCode::kDeadlineExpired:
      return "Deadline Expired";
    case StatusCode::kClientClosedRequest:
      return "Client Closed Request";
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
    case StatusCode::kNotExtended:
      return "Not Extended";
    case StatusCode::kNetworkAuthenticationRequired:
      return "Network Authentication Required";
  }
  return "Unknown status";
}

std::string ToString(StatusCode status) {
  return fmt::format("{} {}", status, StatusCodeString(status));
}

std::ostream& operator<<(std::ostream& os, StatusCode s) {
  switch (s) {
    case StatusCode::kInvalid:
      return os << "Invalid status";
    case StatusCode::kContinue:
      return os << "100 Continue";
    case StatusCode::kSwitchingProtocols:
      return os << "101 Switching Protocols";
    case StatusCode::kProcessing:
      return os << "102 Processing";
    case StatusCode::kEarlyHints:
      return os << "103 Early Hints";
    case StatusCode::kOk:
      return os << "200 OK";
    case StatusCode::kCreated:
      return os << "201 Created";
    case StatusCode::kAccepted:
      return os << "202 Accepted";
    case StatusCode::kNonAuthoritativeInformation:
      return os << "203 Non Authoritative Information";
    case StatusCode::kNoContent:
      return os << "204 NoContent";
    case StatusCode::kResetContent:
      return os << "205 Reset Content";
    case StatusCode::kPartialContent:
      return os << "206 Partial Content";
    case StatusCode::kMultiStatus:
      return os << "207 Multi Status";
    case StatusCode::kAlreadyReported:
      return os << "208 Already Reported";
    case StatusCode::kThisIsFine:
      return os << "218 This Is Fine";
    case StatusCode::kImUsed:
      return os << "226 IM Used";
    case StatusCode::kMultipleChoices:
      return os << "300 Multiple Choices";
    case StatusCode::kMovedPermanently:
      return os << "301 Moved Permanently";
    case StatusCode::kFound:
      return os << "302 Found";
    case StatusCode::kSeeOther:
      return os << "303 See Other";
    case StatusCode::kNotModified:
      return os << "304 Not Modified";
    case StatusCode::kUseProxy:
      return os << "305 Use Proxy";
    case StatusCode::kSwitchProxy:
      return os << "306 Switch Proxy";
    case StatusCode::kTemporaryRedirect:
      return os << "307 Temporary Redirect";
    case StatusCode::kPermanentRedirect:
      return os << "308 Permanent Redirect";
    case StatusCode::kBadRequest:
      return os << "400 BadRequest";
    case StatusCode::kUnauthorized:
      return os << "401 Unauthorized";
    case StatusCode::kPaymentRequired:
      return os << "402 Payment Required";
    case StatusCode::kForbidden:
      return os << "403 Forbidden";
    case StatusCode::kNotFound:
      return os << "404 Not Found";
    case StatusCode::kMethodNotAllowed:
      return os << "405 Method Not Allowed";
    case StatusCode::kNotAcceptable:
      return os << "406 Not Acceptable";
    case StatusCode::kProxyAuthenticationRequired:
      return os << "407 Proxy Authentication Required";
    case StatusCode::kRequestTimeout:
      return os << "408 Request Timeout";
    case StatusCode::kConflict:
      return os << "409 Conflict";
    case StatusCode::kGone:
      return os << "410 Gone";
    case StatusCode::kLengthRequired:
      return os << "411 Length Required";
    case StatusCode::kPreconditionFailed:
      return os << "412 Precondition Failed";
    case StatusCode::kPayloadTooLarge:
      return os << "413 Payload Too Large";
    case StatusCode::kUriTooLong:
      return os << "414 Uri Too Long";
    case StatusCode::kUnsupportedMediaType:
      return os << "415 Unsupported Media Type";
    case StatusCode::kRangeNotSatisfiable:
      return os << "416 Range Not Satisfiable";
    case StatusCode::kExpectationFailed:
      return os << "417 Expectation Failed";
    case StatusCode::kImATeapot:
      return os << "418 I'm A Teapot";
    case StatusCode::kPageExpired:
      return os << "419 Page Expired";
    case StatusCode::kMethodFailure:
      return os << "420 Method Failure";
    case StatusCode::kMisdirectedRequest:
      return os << "421 Misdirected Request";
    case StatusCode::kUnprocessableEntity:
      return os << "422 Unprocessable Entity";
    case StatusCode::kLocked:
      return os << "423 Locked";
    case StatusCode::kFailedDependency:
      return os << "424 Failed Dependency";
    case StatusCode::kTooEarly:
      return os << "425 Too Early";
    case StatusCode::kUpgradeRequired:
      return os << "426 Upgrade Required";
    case StatusCode::kPreconditionRequired:
      return os << "428 Precondition Required";
    case StatusCode::kTooManyRequests:
      return os << "429 Too Many Requests";
    case StatusCode::kRequestHeaderFieldsTooLarge:
      return os << "431 Request Header Fields Too Large";
    case StatusCode::kNginxNoResponse:
      return os << "444 Nginx No Response";
    case StatusCode::kUnavailableForLegalReasons:
      return os << "451 Unavailable For Legal Reasons";
    case StatusCode::kNginxRequestHeaderTooLarge:
      return os << "494 Nginx Request Header Too Large";
    case StatusCode::kNginxSSLCertificateError:
      return os << "495 Nginx SSL Certificate Error";
    case StatusCode::kNginxSSLCertificateRequired:
      return os << "496 Nginx SSL Certificate Required";
    case StatusCode::kNginxHTTPRequestSenttoHTTPSPort:
      return os << "497 Nginx HTTP Request Sentto HTTPS Port";
    case StatusCode::kDeadlineExpired:
      return os << "489 userver specific deadline expired";
    case StatusCode::kNginxClientClosedRequest:
      return os << "499 Nginx Client Closed Request";
    case StatusCode::kInternalServerError:
      return os << "500 Internal Server Error";
    case StatusCode::kNotImplemented:
      return os << "501 Not Implemented";
    case StatusCode::kBadGateway:
      return os << "502 Bad Gateway";
    case StatusCode::kServiceUnavailable:
      return os << "503 Service Unavailable";
    case StatusCode::kGatewayTimeout:
      return os << "504 Gateway Timeout";
    case StatusCode::kHttpVersionNotSupported:
      return os << "505 Http Version Not Supported";
    case StatusCode::kVariantAlsoNegotiates:
      return os << "506 Variant Also Negotiates";
    case StatusCode::kInsufficientStorage:
      return os << "507 Insufficient Storage";
    case StatusCode::kLoopDetected:
      return os << "508 Loop Detected";
    case StatusCode::kBandwidthLimitExceeded:
      return os << "509 Bandwidth Limit Exceeded";
    case StatusCode::kNotExtended:
      return os << "510 Not Extended";
    case StatusCode::kNetworkAuthenticationRequired:
      return os << "511 Network Authentication Required";
    case StatusCode::kWebServerIsDown:
      return os << "520 Web Server Is Down";
    case StatusCode::kConnectionTimedOut:
      return os << "522 Connection Timed Out";
    case StatusCode::kOriginIsUnreachable:
      return os << "523 Origin Is Unreachable";
    case StatusCode::kTimeoutOccurred:
      return os << "524 A Timeout Occurred";
    case StatusCode::kSslHandshakeFailed:
      return os << "525 SSL Handshake Failed";
    case StatusCode::kInvalidSslCertificate:
      return os << "526 Invalid SSL Certificate";
  }

  return os << static_cast<std::underlying_type_t<StatusCode>>(s);
}

}  // namespace http

USERVER_NAMESPACE_END