#pragma once

namespace http {
namespace headers {

// Headers from rfc7231

// Representation Metadata
inline const char kContentType[] = "Content-Type";
inline const char kContentEncoding[] = "Content-Encoding";
inline const char kContentLanguage[] = "Content-Language";
inline const char kContentLocation[] = "Content-Location";

// Payload Semantics
inline const char kContentLength[] = "Content-Length";
inline const char kContentRange[] = "Content-Range";
inline const char kTrailer[] = "Trailer";
inline const char kTransferEncoding[] = "Transfer-Encoding";

// Request Headers - Controls
inline const char kCacheControl[] = "Cache-Control";
inline const char kExpect[] = "Expect";
inline const char kHost[] = "Host";
inline const char kMaxForwards[] = "Max-Forwards";
inline const char kPragma[] = "Pragma";
inline const char kRange[] = "Range";
inline const char kTE[] = "TE";

// Conditionals
inline const char kIfMatch[] = "If-Match";
inline const char kIfNoneMatch[] = "If-None-Match";
inline const char kIfModifiedSince[] = "If-Modified-Since";
inline const char kIfUnmodifiedSince[] = "If-Unmodified-Since";
inline const char kIfRange[] = "If-Range";

// Content Negotiation
inline const char kAccept[] = "Accept";
inline const char kAcceptCharset[] = "Accept-Charset";
inline const char kAcceptEncoding[] = "Accept-Encoding";
inline const char kAcceptLanguage[] = "Accept-Language";

// Authentication Credentials
inline const char kAuthorization[] = "Authorization";
inline const char kProxyAuthorization[] = "Proxy-Authorization";

// Request Context
inline const char kFrom[] = "From";
inline const char kReferer[] = "Referer";
inline const char kUserAgent[] = "User-Agent";

// Response Header Fields

// Control Data
inline const char kAge[] = "Age";
inline const char kExpires[] = "Expires";
inline const char kDate[] = "Date";
inline const char kLocation[] = "Location";
inline const char kRetryAfter[] = "Retry-After";
inline const char kVary[] = "Vary";
inline const char kWarning[] = "Warning";

// Validator Header Fields
inline const char kETag[] = "ETag";
inline const char kLastModified[] = "Last-Modified";

// Authentication Challenges
inline const char kWWWAuthenticate[] = "WWW-Authenticate";
inline const char kProxyAuthenticate[] = "Proxy-Authenticate";

// Response Context
inline const char kAcceptRanges[] = "Accept-Ranges";
inline const char kAllow[] = "Allow";
inline const char kServer[] = "Server";

// Extra headers
inline const char kConnection[] = "Connection";

// Tracing headers

inline const char kXYaRequestId[] = "X-YaRequestId";
inline const char kXYaTraceId[] = "X-YaTraceId";
inline const char kXYaSpanId[] = "X-YaSpanId";

// Generic Yandex headers
inline constexpr char kXYandexUid[] = "X-Yandex-UID";

}  // namespace headers
}  // namespace http
