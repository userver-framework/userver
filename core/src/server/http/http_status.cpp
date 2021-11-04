#include <userver/server/http/http_status.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::http {

std::string HttpStatusString(HttpStatus status) {
  switch (status) {
    case HttpStatus::kContinue:
      return "Continue";
    case HttpStatus::kSwitchingProtocols:
      return "Switching Protocols";
    case HttpStatus::kProcessing:
      return "Processing";
    case HttpStatus::kOk:
      return "OK";
    case HttpStatus::kCreated:
      return "Created";
    case HttpStatus::kAccepted:
      return "Accepted";
    case HttpStatus::kNonAuthoritativeInformation:
      return "Non-Authoritative Information";
    case HttpStatus::kNoContent:
      return "No Content";
    case HttpStatus::kResetContent:
      return "Reset Content";
    case HttpStatus::kPartialContent:
      return "Partial Content";
    case HttpStatus::kMultiStatus:
      return "Multi-Status";
    case HttpStatus::kAlreadyReported:
      return "Already Reported";
    case HttpStatus::kImUsed:
      return "IM Used";
    case HttpStatus::kMultipleChoices:
      return "Multiple Choices";
    case HttpStatus::kMovedPermanently:
      return "Moved Permanently";
    case HttpStatus::kFound:
      return "Found";
    case HttpStatus::kSeeOther:
      return "See Other";
    case HttpStatus::kNotModified:
      return "Not Modified";
    case HttpStatus::kUseProxy:
      return "Use Proxy";
    case HttpStatus::kTemporaryRedirect:
      return "Temporary Redirect";
    case HttpStatus::kPermanentRedirect:
      return "Permanent Redirect";
    case HttpStatus::kBadRequest:
      return "Bad Request";
    case HttpStatus::kUnauthorized:
      return "Unauthorized";
    case HttpStatus::kPaymentRequired:
      return "Payment Required";
    case HttpStatus::kForbidden:
      return "Forbidden";
    case HttpStatus::kNotFound:
      return "Not Found";
    case HttpStatus::kMethodNotAllowed:
      return "Method Not Allowed";
    case HttpStatus::kNotAcceptable:
      return "Not Acceptable";
    case HttpStatus::kProxyAuthenticationRequired:
      return "Proxy Authentication Required";
    case HttpStatus::kRequestTimeout:
      return "Request Timeout";
    case HttpStatus::kConflict:
      return "Conflict";
    case HttpStatus::kGone:
      return "Gone";
    case HttpStatus::kLengthRequired:
      return "Length Required";
    case HttpStatus::kPreconditionFailed:
      return "Precondition Failed";
    case HttpStatus::kPayloadTooLarge:
      return "Payload Too Large";
    case HttpStatus::kUriTooLong:
      return "URI Too Long";
    case HttpStatus::kUnsupportedMediaType:
      return "Unsupported Media Type";
    case HttpStatus::kRangeNotSatisfiable:
      return "Range Not Satisfiable";
    case HttpStatus::kExpectationFailed:
      return "Expectation Failed";
    case HttpStatus::kMisdirectedRequest:
      return "Misdirected Request";
    case HttpStatus::kUnprocessableEntity:
      return "Unprocessable Entity";
    case HttpStatus::kLocked:
      return "Locked";
    case HttpStatus::kFailedDependency:
      return "Failed Dependency";
    case HttpStatus::kUpgradeRequired:
      return "Upgrade Required";
    case HttpStatus::kPreconditionRequired:
      return "Precondition Required";
    case HttpStatus::kTooManyRequests:
      return "Too Many Requests";
    case HttpStatus::kRequestHeaderFieldsTooLarge:
      return "Request Header Fields Too Large";
    case HttpStatus::kUnavailableForLegalReasons:
      return "Unavailable For Legal Reasons";
    case HttpStatus::kClientClosedRequest:
      return "Client Closed Request";
    case HttpStatus::kInternalServerError:
      return "Internal Server Error";
    case HttpStatus::kNotImplemented:
      return "Not Implemented";
    case HttpStatus::kBadGateway:
      return "Bad Gateway";
    case HttpStatus::kServiceUnavailable:
      return "Service Unavailable";
    case HttpStatus::kGatewayTimeout:
      return "Gateway Timeout";
    case HttpStatus::kHttpVersionNotSupported:
      return "HTTP Version Not Supported";
    case HttpStatus::kVariantAlsoNegotiates:
      return "Variant Also Negotiates";
    case HttpStatus::kInsufficientStorage:
      return "Insufficient Storage";
    case HttpStatus::kLoopDetected:
      return "Loop Detected";
    case HttpStatus::kNotExtended:
      return "Not Extended";
    case HttpStatus::kNetworkAuthenticationRequired:
      return "Network Authentication Required";
  }
  return "Unknown status (" + std::to_string(static_cast<int>(status)) + ")";
}

}  // namespace server::http

USERVER_NAMESPACE_END
