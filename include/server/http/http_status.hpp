#pragma once

#include <string>

namespace server {
namespace http {

/* Status Codes */
#define HTTP_STATUS_MAP_STRING(XX)                                           \
  XX(100, kContinue, "Continue")                                             \
  XX(101, kSwitchingProtocols, "Switching Protocols")                        \
  XX(102, kProcessing, "Processing")                                         \
  XX(200, kOk, "OK")                                                         \
  XX(201, kCreated, "Created")                                               \
  XX(202, kAccepted, "Accepted")                                             \
  XX(203, kNonAuthoritativeInformation, "Non-Authoritative Information")     \
  XX(204, kNoContent, "No Content")                                          \
  XX(205, kResetContent, "Reset Content")                                    \
  XX(206, kPartialContent, "Partial Content")                                \
  XX(207, kMultiStatus, "Multi-Status")                                      \
  XX(208, kAlreadyReported, "Already Reported")                              \
  XX(226, kImUsed, "IM Used")                                                \
  XX(300, kMultipleChoices, "Multiple Choices")                              \
  XX(301, kMovedPermanently, "Moved Permanently")                            \
  XX(302, kFound, "Found")                                                   \
  XX(303, kSeeOther, "See Other")                                            \
  XX(304, kNotModified, "Not Modified")                                      \
  XX(305, kUseProxy, "Use Proxy")                                            \
  XX(307, kTemporaryRedirect, "Temporary Redirect")                          \
  XX(308, kPermanentRedirect, "Permanent Redirect")                          \
  XX(400, kBadRequest, "Bad Request")                                        \
  XX(401, kUnauthorized, "Unauthorized")                                     \
  XX(402, kPaymentRequired, "Payment Required")                              \
  XX(403, kForbidden, "Forbidden")                                           \
  XX(404, kNotFound, "Not Found")                                            \
  XX(405, kMethodNotAllowed, "Method Not Allowed")                           \
  XX(406, kNotAcceptable, "Not Acceptable")                                  \
  XX(407, kProxyAuthenticationRequired, "Proxy Authentication Required")     \
  XX(408, kRequestTimeout, "Request Timeout")                                \
  XX(409, kConflict, "Conflict")                                             \
  XX(410, kGone, "Gone")                                                     \
  XX(411, kLengthRequired, "Length Required")                                \
  XX(412, kPreconditionFailed, "Precondition Failed")                        \
  XX(413, kPayloadTooLarge, "Payload Too Large")                             \
  XX(414, kUriTooLong, "URI Too Long")                                       \
  XX(415, kUnsupportedMediaType, "Unsupported Media Type")                   \
  XX(416, kRangeNotSatisfiable, "Range Not Satisfiable")                     \
  XX(417, kExpectationFailed, "Expectation Failed")                          \
  XX(421, kMisdirectedRequest, "Misdirected Request")                        \
  XX(422, kUnprocessableEntity, "Unprocessable Entity")                      \
  XX(423, kLocked, "Locked")                                                 \
  XX(424, kFailedDependency, "Failed Dependency")                            \
  XX(426, kUpgradeRequired, "Upgrade Required")                              \
  XX(428, kPreconditionRequired, "Precondition Required")                    \
  XX(429, kTooManyRequests, "Too Many Requests")                             \
  XX(431, kRequestHeaderFieldsTooLarge, "Request Header Fields Too Large")   \
  XX(451, kUnavailableForLegalReasons, "Unavailable For Legal Reasons")      \
  XX(499, kClientClosedRequest, "Client Closed Request")                     \
  XX(500, kInternalServerError, "Internal Server Error")                     \
  XX(501, kNotImplemented, "Not Implemented")                                \
  XX(502, kBadGateway, "Bad Gateway")                                        \
  XX(503, kServiceUnavailable, "Service Unavailable")                        \
  XX(504, kGatewayTimeout, "Gateway Timeout")                                \
  XX(505, kHttpVersionNotSupported, "HTTP Version Not Supported")            \
  XX(506, kVariantAlsoNegotiates, "Variant Also Negotiates")                 \
  XX(507, kInsufficientStorage, "Insufficient Storage")                      \
  XX(508, kLoopDetected, "Loop Detected")                                    \
  XX(510, kNotExtended, "Not Extended")                                      \
  XX(511, kNetworkAuthenticationRequired, "Network Authentication Required") \
  /* end of HTTP_STATUS_MAP_STRING */

enum class HttpStatus {
#define XX(num, name, string) name = num,

  HTTP_STATUS_MAP_STRING(XX)

#undef XX
};

inline std::string HttpStatusString(HttpStatus status) {
  switch (status) {
#define XX(num, name, string) \
  case HttpStatus::name:      \
    return string;

    HTTP_STATUS_MAP_STRING(XX)

#undef XX
  }
  return "Unknown status (" + std::to_string(static_cast<int>(status)) + ")";
}

#undef HTTP_STATUS_MAP_STRING

}  // namespace http
}  // namespace server
