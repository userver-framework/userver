#pragma once

/// @file http/common_headers.hpp
/// @brief Common HTTP header names

namespace http::headers {

// Headers from rfc7231

/// @name Representation Metadata
/// @{
inline const char kContentType[] = "Content-Type";
inline const char kContentEncoding[] = "Content-Encoding";
inline const char kContentLanguage[] = "Content-Language";
inline const char kContentLocation[] = "Content-Location";
/// @}

/// @name Payload Semantics
/// @{
inline const char kContentLength[] = "Content-Length";
inline const char kContentRange[] = "Content-Range";
inline const char kTrailer[] = "Trailer";
inline const char kTransferEncoding[] = "Transfer-Encoding";
/// @}

/// @name Request Headers - Controls
/// @{
inline const char kCacheControl[] = "Cache-Control";
inline const char kExpect[] = "Expect";
inline const char kHost[] = "Host";
inline const char kMaxForwards[] = "Max-Forwards";
inline const char kPragma[] = "Pragma";
inline const char kRange[] = "Range";
inline const char kTE[] = "TE";
/// @}

/// @name Conditionals
/// @{
inline const char kIfMatch[] = "If-Match";
inline const char kIfNoneMatch[] = "If-None-Match";
inline const char kIfModifiedSince[] = "If-Modified-Since";
inline const char kIfUnmodifiedSince[] = "If-Unmodified-Since";
inline const char kIfRange[] = "If-Range";
/// @}

/// @name Content Negotiation
/// @{
inline const char kAccept[] = "Accept";
inline const char kAcceptCharset[] = "Accept-Charset";
inline const char kAcceptEncoding[] = "Accept-Encoding";
inline const char kAcceptLanguage[] = "Accept-Language";
/// @}

/// @name Authentication Credentials
/// @{
inline const char kAuthorization[] = "Authorization";
inline const char kProxyAuthorization[] = "Proxy-Authorization";
/// @}

/// @name Request Context
/// @{
inline const char kFrom[] = "From";
inline const char kReferer[] = "Referer";
inline const char kUserAgent[] = "User-Agent";
/// @}

// Response Header Fields

/// @name Control Data
/// @{
inline const char kAge[] = "Age";
inline const char kExpires[] = "Expires";
inline const char kDate[] = "Date";
inline const char kLocation[] = "Location";
inline const char kRetryAfter[] = "Retry-After";
inline const char kVary[] = "Vary";
inline const char kWarning[] = "Warning";
inline const char kAccessControlAllowHeaders[] = "Access-Control-Allow-Headers";
/// @}

/// @name Validator Header Fields
/// @{
inline const char kETag[] = "ETag";
inline const char kLastModified[] = "Last-Modified";
/// @}

/// @name Authentication Challenges
/// @{
inline const char kWWWAuthenticate[] = "WWW-Authenticate";
inline const char kProxyAuthenticate[] = "Proxy-Authenticate";
/// @}

/// @name Response Context
/// @{
inline const char kAcceptRanges[] = "Accept-Ranges";
inline const char kAllow[] = "Allow";
inline const char kServer[] = "Server";
/// @}

/// @name Cookie
/// @{
inline const char kSetCookie[] = "Set-Cookie";
/// @}

/// @name Extra headers
/// @{
inline const char kConnection[] = "Connection";
/// @}

/// @name Tracing headers
/// @{
inline const char kXYaRequestId[] = "X-YaRequestId";
inline const char kXYaTraceId[] = "X-YaTraceId";
inline const char kXYaSpanId[] = "X-YaSpanId";
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
/// @}

}  // namespace http::headers
