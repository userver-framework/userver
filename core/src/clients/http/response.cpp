#include <userver/clients/http/response.hpp>

#include <ostream>

#include <userver/clients/http/response_future.hpp>

USERVER_NAMESPACE_BEGIN

namespace clients::http {

std::ostream& operator<<(std::ostream& os, Status s) {
  switch (s) {
    case Invalid:
      return os << "Invalid status";
    case Continue:
      return os << "100 Continue";
    case SwitchingProtocols:
      return os << "101 Switching Protocols";
    case Processing:
      return os << "102 Processing";
    case EarlyHints:
      return os << "103 Early Hints";
    case OK:
      return os << "200 OK";
    case Created:
      return os << "201 Created";
    case Accepted:
      return os << "202 Accepted";
    case NonAuthoritativeInformation:
      return os << "203 Non Authoritative Information";
    case NoContent:
      return os << "204 NoContent";
    case ResetContent:
      return os << "205 Reset Content";
    case PartialContent:
      return os << "206 Partial Content";
    case MultiStatus:
      return os << "207 Multi Status";
    case AlreadyReported:
      return os << "208 Already Reported";
    case ThisIsFine:
      return os << "218 This Is Fine";
    case IMUsed:
      return os << "226 IM Used";
    case MultipleChoices:
      return os << "300 Multiple Choices";
    case MovedPermanently:
      return os << "301 Moved Permanently";
    case Found:
      return os << "302 Found";
    case SeeOther:
      return os << "303 See Other";
    case NotModified:
      return os << "304 Not Modified";
    case UseProxy:
      return os << "305 Use Proxy";
    case SwitchProxy:
      return os << "306 Switch Proxy";
    case TemporaryRedirect:
      return os << "307 Temporary Redirect";
    case PermanentRedirect:
      return os << "308 Permanent Redirect";
    case BadRequest:
      return os << "400 BadRequest";
    case Unauthorized:
      return os << "401 Unauthorized";
    case PaymentRequired:
      return os << "402 Payment Required";
    case Forbidden:
      return os << "403 Forbidden";
    case NotFound:
      return os << "404 Not Found";
    case MethodNotAllowed:
      return os << "405 Method Not Allowed";
    case NotAcceptable:
      return os << "406 Not Acceptable";
    case ProxyAuthenticationRequired:
      return os << "407 Proxy Authentication Required";
    case RequestTimeout:
      return os << "408 Request Timeout";
    case Conflict:
      return os << "409 Conflict";
    case Gone:
      return os << "410 Gone";
    case LengthRequired:
      return os << "411 Length Required";
    case PreconditionFailed:
      return os << "412 Precondition Failed";
    case PayloadTooLarge:
      return os << "413 Payload Too Large";
    case UriTooLong:
      return os << "414 Uri Too Long";
    case UnsupportedMediaType:
      return os << "415 Unsupported Media Type";
    case RangeNotSatisfiable:
      return os << "416 Range Not Satisfiable";
    case ExpectationFailed:
      return os << "417 Expectation Failed";
    case ImATeapot:
      return os << "418 I'm A Teapot";
    case PageExpired:
      return os << "419 Page Expired";
    case MethodFailure:
      return os << "420 Method Failure";
    case MisdirectedRequest:
      return os << "421 Misdirected Request";
    case UnprocessableEntity:
      return os << "422 Unprocessable Entity";
    case Locked:
      return os << "423 Locked";
    case FailedDependency:
      return os << "424 Failed Dependency";
    case TooEarly:
      return os << "425 Too Early";
    case UpgradeRequired:
      return os << "426 Upgrade Required";
    case PreconditionRequired:
      return os << "428 Precondition Required";
    case TooManyRequests:
      return os << "429 Too Many Requests";
    case RequestHeaderFieldsTooLarge:
      return os << "431 Request Header Fields Too Large";
    case NginxNoResponse:
      return os << "444 Nginx No Response";
    case UnavailableForLegalReasons:
      return os << "451 Unavailable For Legal Reasons";
    case NginxRequestHeaderTooLarge:
      return os << "494 Nginx Request Header Too Large";
    case NginxSSLCertificateError:
      return os << "495 Nginx SSL Certificate Error";
    case NginxSSLCertificateRequired:
      return os << "496 Nginx SSL Certificate Required";
    case NginxHTTPRequestSenttoHTTPSPort:
      return os << "497 Nginx HTTP Request Sentto HTTPS Port";
    case NginxClientClosedRequest:
      return os << "499 Nginx Client Closed Request";
    case InternalServerError:
      return os << "500 Internal Server Error";
    case NotImplemented:
      return os << "501 Not Implemented";
    case BadGateway:
      return os << "502 Bad Gateway";
    case ServiceUnavailable:
      return os << "503 Service Unavailable";
    case GatewayTimeout:
      return os << "504 Gateway Timeout";
    case HttpVersionNotSupported:
      return os << "505 Http Version Not Supported";
    case VariantAlsoNegotiates:
      return os << "506 Variant Also Negotiates";
    case InsufficientStorage:
      return os << "507 Insufficient Storage";
    case LoopDetected:
      return os << "508 Loop Detected";
    case BandwidthLimitExceeded:
      return os << "509 Bandwidth Limit Exceeded";
    case NotExtended:
      return os << "510 Not Extended";
    case NetworkAuthenticationRequired:
      return os << "511 Network Authentication Required";
    case WebServerIsDown:
      return os << "520 Web Server Is Down";
    case ConnectionTimedOut:
      return os << "522 Connection Timed Out";
    case OriginIsUnreachable:
      return os << "523 Origin Is Unreachable";
    case TimeoutOccurred:
      return os << "524 A Timeout Occurred";
    case SslHandshakeFailed:
      return os << "525 SSL Handshake Failed";
    case InvalidSslCertificate:
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
