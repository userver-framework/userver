#pragma once

/// @file userver/http/common_headers.hpp
/// @brief Common HTTP header names

/// Common HTTP headers
USERVER_NAMESPACE_BEGIN

namespace http::headers {

// Headers from rfc7231

/// @name Representation Metadata
/// @{
inline constexpr char kContentType[] = "Content-Type";
inline constexpr char kContentEncoding[] = "Content-Encoding";
inline constexpr char kContentLanguage[] = "Content-Language";
inline constexpr char kContentLocation[] = "Content-Location";
inline constexpr char kContentDisposition[] = "Content-Disposition";
/// @}

/// @name Payload Semantics
/// @{
inline constexpr char kContentLength[] = "Content-Length";
inline constexpr char kContentRange[] = "Content-Range";
inline constexpr char kTrailer[] = "Trailer";
inline constexpr char kTransferEncoding[] = "Transfer-Encoding";
/// @}

/// @name Request Headers - Controls
/// @{
inline constexpr char kCacheControl[] = "Cache-Control";
inline constexpr char kExpect[] = "Expect";
inline constexpr char kHost[] = "Host";
inline constexpr char kMaxForwards[] = "Max-Forwards";
inline constexpr char kPragma[] = "Pragma";
inline constexpr char kRange[] = "Range";
inline constexpr char kTE[] = "TE";
/// @}

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
inline constexpr char kAccept[] = "Accept";
inline constexpr char kAcceptCharset[] = "Accept-Charset";
inline constexpr char kAcceptEncoding[] = "Accept-Encoding";
inline constexpr char kAcceptLanguage[] = "Accept-Language";
/// @}

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
inline constexpr char kUserAgent[] = "User-Agent";
/// @}

// Response Header Fields

/// @name Control Data
/// @{
inline constexpr char kAge[] = "Age";
inline constexpr char kExpires[] = "Expires";
inline constexpr char kDate[] = "Date";
inline constexpr char kLocation[] = "Location";
inline constexpr char kRetryAfter[] = "Retry-After";
inline constexpr char kVary[] = "Vary";
inline constexpr char kWarning[] = "Warning";
inline constexpr char kAccessControlAllowHeaders[] =
    "Access-Control-Allow-Headers";
/// @}

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
inline constexpr char kServer[] = "Server";
/// @}

/// @name Cookie
/// @{
inline constexpr char kSetCookie[] = "Set-Cookie";
/// @}

/// @name Extra headers
/// @{
inline constexpr char kConnection[] = "Connection";
/// @}

/// @name Tracing headers
/// @{
inline constexpr char kXYaRequestId[] = "X-YaRequestId";
inline constexpr char kXYaTraceId[] = "X-YaTraceId";
inline constexpr char kXYaSpanId[] = "X-YaSpanId";
/// @}

/// @name Generic Yandex headers
/// @{
inline constexpr char kXYandexUid[] = "X-Yandex-UID";

// IP address of mobile client, not an IP address of single-hop client.
inline constexpr char kXRemoteIp[] = "X-Remote-IP";
/// @}

/// @name Service Yandex.Taxi headers
/// @{
inline constexpr char kXYaTaxiAllowAuthRequest[] =
    "X-YaTaxi-Allow-Auth-Request";
inline constexpr char kXYaTaxiAllowAuthResponse[] =
    "X-YaTaxi-Allow-Auth-Response";
inline constexpr char kXYaTaxiServerHostname[] = "X-YaTaxi-Server-Hostname";
inline constexpr char kXYaTaxiClientTimeoutMs[] = "X-YaTaxi-Client-TimeoutMs";
/// @}

}  // namespace http::headers

USERVER_NAMESPACE_END
