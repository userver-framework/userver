#pragma once

#include <userver/http/predefined_header.hpp>

/// @file userver/http/common_headers.hpp
/// @brief Common HTTP header names

USERVER_NAMESPACE_BEGIN

/// Common HTTP headers
namespace http::headers {

// Headers from rfc7231

/// @name Representation Metadata
/// @{
inline constexpr PredefinedHeader kContentType{"Content-Type"};
inline constexpr PredefinedHeader kContentEncoding{"Content-Encoding"};
inline constexpr PredefinedHeader kContentLanguage{"Content-Language"};
inline constexpr PredefinedHeader kContentLocation{"Content-Location"};
inline constexpr PredefinedHeader kContentDisposition{"Content-Disposition"};
/// @}

/// @name Payload Semantics
/// @{
inline constexpr PredefinedHeader kContentLength{"Content-Length"};
inline constexpr PredefinedHeader kContentRange{"Content-Range"};
inline constexpr PredefinedHeader kTrailer{"Trailer"};
inline constexpr PredefinedHeader kTransferEncoding{"Transfer-Encoding"};
/// @}

/// @name Request Headers - Controls
/// @{
inline constexpr PredefinedHeader kCacheControl{"Cache-Control"};
inline constexpr PredefinedHeader kExpect{"Expect"};
inline constexpr PredefinedHeader kHost{"Host"};
inline constexpr PredefinedHeader kMaxForwards{"Max-Forwards"};
inline constexpr PredefinedHeader kPragma{"Pragma"};
inline constexpr PredefinedHeader kRange{"Range"};
inline constexpr PredefinedHeader kTE{"TE"};
/// @}

/// @name Conditionals
/// @{
inline constexpr PredefinedHeader kIfMatch{"If-Match"};
inline constexpr PredefinedHeader kIfNoneMatch{"If-None-Match"};
inline constexpr PredefinedHeader kIfModifiedSince{"If-Modified-Since"};
inline constexpr PredefinedHeader kIfUnmodifiedSince{"If-Unmodified-Since"};
inline constexpr PredefinedHeader kIfRange{"If-Range"};
/// @}

/// @name Content Negotiation
/// @{
inline constexpr PredefinedHeader kAccept{"Accept"};
inline constexpr PredefinedHeader kAcceptCharset{"Accept-Charset"};
inline constexpr PredefinedHeader kAcceptEncoding{"Accept-Encoding"};
inline constexpr PredefinedHeader kAcceptLanguage{"Accept-Language"};
/// @}

/// @name Authentication Credentials
/// @{
inline constexpr PredefinedHeader kAuthorization{"Authorization"};
inline constexpr PredefinedHeader kProxyAuthorization{"Proxy-Authorization"};
inline constexpr PredefinedHeader kApiKey{"X-YaTaxi-API-Key"};
inline constexpr PredefinedHeader kExternalService{"X-YaTaxi-External-Service"};
/// @}

/// @name Request Context
/// @{
inline constexpr PredefinedHeader kFrom{"From"};
inline constexpr PredefinedHeader kReferer{"Referer"};
inline constexpr PredefinedHeader kUserAgent{"User-Agent"};
inline constexpr PredefinedHeader kXTaxi{"X-Taxi"};
inline constexpr PredefinedHeader kXRequestedUri{"X-Requested-Uri"};
inline constexpr PredefinedHeader kXRequestApplication{"X-Request-Application"};
/// @}

// Response Header Fields

/// @name Control Data
/// @{
inline constexpr PredefinedHeader kAge{"Age"};
inline constexpr PredefinedHeader kExpires{"Expires"};
inline constexpr PredefinedHeader kDate{"Date"};
inline constexpr PredefinedHeader kLocation{"Location"};
inline constexpr PredefinedHeader kRetryAfter{"Retry-After"};
inline constexpr PredefinedHeader kVary{"Vary"};
inline constexpr PredefinedHeader kWarning{"Warning"};
inline constexpr PredefinedHeader kAccessControlAllowHeaders{
    "Access-Control-Allow-Headers"};
/// @}

/// @name Validator Header Fields
/// @{
inline constexpr PredefinedHeader kETag{"ETag"};
inline constexpr PredefinedHeader kLastModified{"Last-Modified"};
/// @}

/// @name Authentication Challenges
/// @{
inline constexpr PredefinedHeader kWWWAuthenticate{"WWW-Authenticate"};
inline constexpr PredefinedHeader kProxyAuthenticate{"Proxy-Authenticate"};
/// @}

/// @name Response Context
/// @{
inline constexpr PredefinedHeader kAcceptRanges{"Accept-Ranges"};
inline constexpr PredefinedHeader kAllow{"Allow"};
inline constexpr PredefinedHeader kServer{"Server"};
/// @}

/// @name Cookie
/// @{
inline constexpr PredefinedHeader kSetCookie{"Set-Cookie"};
/// @}

/// @name Extra headers
/// @{
inline constexpr PredefinedHeader kConnection{"Connection"};
inline constexpr PredefinedHeader kCookie{"Cookie"};
/// @}

/// @name Tracing headers
/// @{
inline constexpr PredefinedHeader kXYaRequestId{"X-YaRequestId"};
inline constexpr PredefinedHeader kXYaTraceId{"X-YaTraceId"};
inline constexpr PredefinedHeader kXYaSpanId{"X-YaSpanId"};
inline constexpr PredefinedHeader kXRequestId{"X-RequestId"};
inline constexpr PredefinedHeader kXBackendServer{"X-Backend-Server"};
inline constexpr PredefinedHeader kXTaxiEnvoyProxyDstVhost{
    "X-Taxi-EnvoyProxy-DstVhost"};
/// @}

/// @name Baggage header
/// @{
inline constexpr PredefinedHeader kXBaggage{"baggage"};
/// @}

/// @name Generic Yandex headers
/// @{
inline constexpr PredefinedHeader kXYandexUid{"X-Yandex-UID"};

// IP address of mobile client, not an IP address of single-hop client.
inline constexpr PredefinedHeader kXRemoteIp{"X-Remote-IP"};
/// @}

/// @name Generic Yandex/MLU headers
/// @{
inline constexpr PredefinedHeader kXYaTaxiAllowAuthRequest{
    "X-YaTaxi-Allow-Auth-Request"};
inline constexpr PredefinedHeader kXYaTaxiAllowAuthResponse{
    "X-YaTaxi-Allow-Auth-Response"};
inline constexpr PredefinedHeader kXYaTaxiServerHostname{
    "X-YaTaxi-Server-Hostname"};
inline constexpr PredefinedHeader kXYaTaxiClientTimeoutMs{
    "X-YaTaxi-Client-TimeoutMs"};
inline constexpr PredefinedHeader kXYaTaxiRatelimitedBy{
    "X-YaTaxi-Ratelimited-By"};
inline constexpr PredefinedHeader kXYaTaxiRatelimitReason{
    "X-YaTaxi-Ratelimit-Reason"};

namespace ratelimit_reason {
inline constexpr std::string_view kCC{"congestion-control"};
inline constexpr std::string_view kMaxResponseSizeInFlight{
    "max-response-size-in-flight"};
inline constexpr std::string_view kMaxPendingResponses{
    "too-many-pending-responses"};
inline constexpr std::string_view kGlobal{"global-ratelimit"};
inline constexpr std::string_view kInFlight{"max-requests-in-flight"};
}  // namespace ratelimit_reason
/// @}

}  // namespace http::headers

USERVER_NAMESPACE_END
