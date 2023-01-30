#pragma once

/// @file userver/http/common_headers.hpp
/// @brief Common HTTP header names

USERVER_NAMESPACE_BEGIN

/// Common HTTP headers
namespace http::headers {

namespace impl {
constexpr bool IsLowerCase(std::string_view header) {
  for (const auto c : header) {
    if (c >= 'A' && c <= 'Z') {
      return false;
    }
  }

  return true;
}
}  // namespace impl

// Headers from rfc7231

/// @name Representation Metadata
/// @{
inline constexpr char kContentType[] = "content-type";
inline constexpr char kContentEncoding[] = "Content-Encoding";
inline constexpr char kContentLanguage[] = "Content-Language";
inline constexpr char kContentLocation[] = "Content-Location";
inline constexpr char kContentDisposition[] = "Content-Disposition";
/// @}

static_assert(impl::IsLowerCase(kContentType));

/// @name Payload Semantics
/// @{
inline constexpr char kContentLength[] = "content-length";
inline constexpr char kContentRange[] = "Content-Range";
inline constexpr char kTrailer[] = "Trailer";
inline constexpr char kTransferEncoding[] = "Transfer-Encoding";
/// @}

static_assert(impl::IsLowerCase(kContentLength));

/// @name Request Headers - Controls
/// @{
inline constexpr char kCacheControl[] = "Cache-Control";
inline constexpr char kExpect[] = "Expect";
inline constexpr char kHost[] = "host";
inline constexpr char kMaxForwards[] = "Max-Forwards";
inline constexpr char kPragma[] = "Pragma";
inline constexpr char kRange[] = "Range";
inline constexpr char kTE[] = "TE";
/// @}

static_assert(impl::IsLowerCase(kHost));

/// @name Conditionals
/// @{
inline constexpr char kIfMatch[] = "If-Match";
inline constexpr char kIfNoneMatch[] = "If-None-Match";
inline constexpr char kIfModifiedSince[] = "If-Modified-Since";
inline constexpr char kIfUnmodifiedSince[] = "If-Unmodified-Since";
inline constexpr char kIfRange[] = "If-Range";
/// @}

/// @name Content Negotiation
/// @{
inline constexpr char kAccept[] = "accept";
inline constexpr char kAcceptCharset[] = "Accept-Charset";
inline constexpr char kAcceptEncoding[] = "accept-encoding";
inline constexpr char kAcceptLanguage[] = "Accept-Language";
/// @}

static_assert(impl::IsLowerCase(kAccept));
static_assert(impl::IsLowerCase(kAcceptEncoding));

/// @name Authentication Credentials
/// @{
inline constexpr char kAuthorization[] = "Authorization";
inline constexpr char kProxyAuthorization[] = "Proxy-Authorization";
inline constexpr char kApiKey[] = "X-YaTaxi-API-Key";
inline constexpr char kExternalService[] = "X-YaTaxi-External-Service";
/// @}

/// @name Request Context
/// @{
inline constexpr char kFrom[] = "From";
inline constexpr char kReferer[] = "Referer";
inline constexpr char kUserAgent[] = "user-agent";
inline constexpr char kXTaxi[] = "X-Taxi";
inline constexpr char kXRequestedUri[] = "X-Requested-Uri";
inline constexpr char kXRequestApplication[] = "X-Request-Application";
/// @}

static_assert(impl::IsLowerCase(kUserAgent));

// Response Header Fields

/// @name Control Data
/// @{
inline constexpr char kAge[] = "Age";
inline constexpr char kExpires[] = "Expires";
inline constexpr char kDate[] = "date";
inline constexpr char kLocation[] = "Location";
inline constexpr char kRetryAfter[] = "Retry-After";
inline constexpr char kVary[] = "Vary";
inline constexpr char kWarning[] = "Warning";
inline constexpr char kAccessControlAllowHeaders[] =
    "Access-Control-Allow-Headers";
/// @}

static_assert(impl::IsLowerCase(kDate));

/// @name Validator Header Fields
/// @{
inline constexpr char kETag[] = "ETag";
inline constexpr char kLastModified[] = "Last-Modified";
/// @}

/// @name Authentication Challenges
/// @{
inline constexpr char kWWWAuthenticate[] = "WWW-Authenticate";
inline constexpr char kProxyAuthenticate[] = "Proxy-Authenticate";
/// @}

/// @name Response Context
/// @{
inline constexpr char kAcceptRanges[] = "Accept-Ranges";
inline constexpr char kAllow[] = "Allow";
inline constexpr char kServer[] = "server";
/// @}

static_assert(impl::IsLowerCase(kServer));

/// @name Cookie
/// @{
inline constexpr char kSetCookie[] = "Set-Cookie";
/// @}

/// @name Extra headers
/// @{
inline constexpr char kConnection[] = "connection";
/// @}

static_assert(impl::IsLowerCase(kConnection));

/// @name Tracing headers
/// @{
inline constexpr char kXYaRequestId[] = "x-yarequestid";
inline constexpr char kXYaTraceId[] = "x-yatraceid";
inline constexpr char kXYaSpanId[] = "x-yaspanid";
inline constexpr char kXRequestId[] = "x-requestid";
inline constexpr char kXBackendServer[] = "x-backend-server";
inline constexpr char kXTaxiEnvoyProxyDstVhost[] = "x-taxi-envoyproxy-dstvhost";
/// @}

static_assert(impl::IsLowerCase(kXYaRequestId));
static_assert(impl::IsLowerCase(kXYaTraceId));
static_assert(impl::IsLowerCase(kXYaSpanId));

/// @name Generic Yandex headers
/// @{
inline constexpr char kXYandexUid[] = "X-Yandex-UID";

// IP address of mobile client, not an IP address of single-hop client.
inline constexpr char kXRemoteIp[] = "X-Remote-IP";
/// @}

/// @name Generic Yandex/MLU headers
/// @{
inline constexpr char kXYaTaxiAllowAuthRequest[] =
    "X-YaTaxi-Allow-Auth-Request";
inline constexpr char kXYaTaxiAllowAuthResponse[] =
    "X-YaTaxi-Allow-Auth-Response";
inline constexpr char kXYaTaxiServerHostname[] = "X-YaTaxi-Server-Hostname";
inline constexpr char kXYaTaxiClientTimeoutMs[] = "x-yaxaxi-client-timeoutms";
inline constexpr char kXYaTaxiRatelimitedBy[] = "X-YaTaxi-Ratelimited-By";
inline constexpr char kXYaTaxiRatelimitReason[] = "X-YaTaxi-Ratelimit-Reason";

namespace ratelimit_reason {
inline constexpr char kCC[] = "congestion-control";
inline constexpr char kMaxResponseSizeInFlight[] =
    "max-response-size-in-flight";
inline constexpr char kMaxPendingResponses[] = "too-many-pending-responses";
inline constexpr char kGlobal[] = "global-ratelimit";
inline constexpr char kInFlight[] = "max-requests-in-flight";
}  // namespace ratelimit_reason
/// @}

}  // namespace http::headers

USERVER_NAMESPACE_END
