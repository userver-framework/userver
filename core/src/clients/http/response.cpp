#include <userver/clients/http/response.hpp>

#include <ostream>

#include <userver/clients/http/response_future.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

std::ostream& operator<<(std::ostream& os, Status s) {
  switch (s) {
    case Status::kInvalid:
      return os << "Invalid status";
    case Status::kContinue:
      return os << "100 Continue";
    case Status::kSwitchingProtocols:
      return os << "101 Switching Protocols";
    case Status::kProcessing:
      return os << "102 Processing";
    case Status::kEarlyHints:
      return os << "103 Early Hints";
    case Status::kOk:
      return os << "200 OK";
    case Status::kCreated:
      return os << "201 Created";
    case Status::kAccepted:
      return os << "202 Accepted";
    case Status::kNonAuthoritativeInformation:
      return os << "203 Non Authoritative Information";
    case Status::kNoContent:
      return os << "204 NoContent";
    case Status::kResetContent:
      return os << "205 Reset Content";
    case Status::kPartialContent:
      return os << "206 Partial Content";
    case Status::kMultiStatus:
      return os << "207 Multi Status";
    case Status::kAlreadyReported:
      return os << "208 Already Reported";
    case Status::kThisIsFine:
      return os << "218 This Is Fine";
    case Status::kImUsed:
      return os << "226 IM Used";
    case Status::kMultipleChoices:
      return os << "300 Multiple Choices";
    case Status::kMovedPermanently:
      return os << "301 Moved Permanently";
    case Status::kFound:
      return os << "302 Found";
    case Status::kSeeOther:
      return os << "303 See Other";
    case Status::kNotModified:
      return os << "304 Not Modified";
    case Status::kUseProxy:
      return os << "305 Use Proxy";
    case Status::kSwitchProxy:
      return os << "306 Switch Proxy";
    case Status::kTemporaryRedirect:
      return os << "307 Temporary Redirect";
    case Status::kPermanentRedirect:
      return os << "308 Permanent Redirect";
    case Status::kBadRequest:
      return os << "400 BadRequest";
    case Status::kUnauthorized:
      return os << "401 Unauthorized";
    case Status::kPaymentRequired:
      return os << "402 Payment Required";
    case Status::kForbidden:
      return os << "403 Forbidden";
    case Status::kNotFound:
      return os << "404 Not Found";
    case Status::kMethodNotAllowed:
      return os << "405 Method Not Allowed";
    case Status::kNotAcceptable:
      return os << "406 Not Acceptable";
    case Status::kProxyAuthenticationRequired:
      return os << "407 Proxy Authentication Required";
    case Status::kRequestTimeout:
      return os << "408 Request Timeout";
    case Status::kConflict:
      return os << "409 Conflict";
    case Status::kGone:
      return os << "410 Gone";
    case Status::kLengthRequired:
      return os << "411 Length Required";
    case Status::kPreconditionFailed:
      return os << "412 Precondition Failed";
    case Status::kPayloadTooLarge:
      return os << "413 Payload Too Large";
    case Status::kUriTooLong:
      return os << "414 Uri Too Long";
    case Status::kUnsupportedMediaType:
      return os << "415 Unsupported Media Type";
    case Status::kRangeNotSatisfiable:
      return os << "416 Range Not Satisfiable";
    case Status::kExpectationFailed:
      return os << "417 Expectation Failed";
    case Status::kImATeapot:
      return os << "418 I'm A Teapot";
    case Status::kPageExpired:
      return os << "419 Page Expired";
    case Status::kMethodFailure:
      return os << "420 Method Failure";
    case Status::kMisdirectedRequest:
      return os << "421 Misdirected Request";
    case Status::kUnprocessableEntity:
      return os << "422 Unprocessable Entity";
    case Status::kLocked:
      return os << "423 Locked";
    case Status::kFailedDependency:
      return os << "424 Failed Dependency";
    case Status::kTooEarly:
      return os << "425 Too Early";
    case Status::kUpgradeRequired:
      return os << "426 Upgrade Required";
    case Status::kPreconditionRequired:
      return os << "428 Precondition Required";
    case Status::kTooManyRequests:
      return os << "429 Too Many Requests";
    case Status::kRequestHeaderFieldsTooLarge:
      return os << "431 Request Header Fields Too Large";
    case Status::kNginxNoResponse:
      return os << "444 Nginx No Response";
    case Status::kUnavailableForLegalReasons:
      return os << "451 Unavailable For Legal Reasons";
    case Status::kNginxRequestHeaderTooLarge:
      return os << "494 Nginx Request Header Too Large";
    case Status::kNginxSSLCertificateError:
      return os << "495 Nginx SSL Certificate Error";
    case Status::kNginxSSLCertificateRequired:
      return os << "496 Nginx SSL Certificate Required";
    case Status::kNginxHTTPRequestSenttoHTTPSPort:
      return os << "497 Nginx HTTP Request Sentto HTTPS Port";
    case Status::kDeadlineExpired:
      return os << "489 userver specific deadline expired";
    case Status::kNginxClientClosedRequest:
      return os << "499 Nginx Client Closed Request";
    case Status::kInternalServerError:
      return os << "500 Internal Server Error";
    case Status::kNotImplemented:
      return os << "501 Not Implemented";
    case Status::kBadGateway:
      return os << "502 Bad Gateway";
    case Status::kServiceUnavailable:
      return os << "503 Service Unavailable";
    case Status::kGatewayTimeout:
      return os << "504 Gateway Timeout";
    case Status::kHttpVersionNotSupported:
      return os << "505 Http Version Not Supported";
    case Status::kVariantAlsoNegotiates:
      return os << "506 Variant Also Negotiates";
    case Status::kInsufficientStorage:
      return os << "507 Insufficient Storage";
    case Status::kLoopDetected:
      return os << "508 Loop Detected";
    case Status::kBandwidthLimitExceeded:
      return os << "509 Bandwidth Limit Exceeded";
    case Status::kNotExtended:
      return os << "510 Not Extended";
    case Status::kNetworkAuthenticationRequired:
      return os << "511 Network Authentication Required";
    case Status::kWebServerIsDown:
      return os << "520 Web Server Is Down";
    case Status::kConnectionTimedOut:
      return os << "522 Connection Timed Out";
    case Status::kOriginIsUnreachable:
      return os << "523 Origin Is Unreachable";
    case Status::kTimeoutOccurred:
      return os << "524 A Timeout Occurred";
    case Status::kSslHandshakeFailed:
      return os << "525 SSL Handshake Failed";
    case Status::kInvalidSslCertificate:
      return os << "526 Invalid SSL Certificate";
  }

  return os << static_cast<std::underlying_type_t<Status>>(s);
}

Status Response::status_code() const { return status_code_; }

void Response::RaiseForStatus(int code, const LocalStats& stats) {
  if (400 <= code && code < 500)
    throw HttpClientException(code, stats);
  else if (500 <= code && code < 600)
    throw HttpServerException(code, stats);
}

void Response::raise_for_status() const {
  RaiseForStatus(status_code(), GetStats());
}

LocalStats Response::GetStats() const { return stats_; }

}  // namespace clients::http

USERVER_NAMESPACE_END
