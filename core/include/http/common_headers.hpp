#pragma once

#include <string>

namespace http {
namespace headers {

// Headers from rfc7231

// Representation Metadata
extern const std::string kContentType;
extern const std::string kContentEncoding;
extern const std::string kContentLanguage;
extern const std::string kContentLocation;

// Payload Semantics
extern const std::string kContentLength;
extern const std::string kContentRange;
extern const std::string kTrailer;
extern const std::string kTransferEncoding;

// Request Headers Controls
extern const std::string kCacheControl;
extern const std::string kExpect;
extern const std::string kHost;
extern const std::string kMaxForwards;
extern const std::string kPragma;
extern const std::string kRange;
extern const std::string kTE;

// Conditionals
extern const std::string kIfMatch;
extern const std::string kIfNoneMatch;
extern const std::string kIfModifiedSince;
extern const std::string kIfUnmodifiedSince;
extern const std::string kIfRange;

// Content Negotiation
extern const std::string kAccept;
extern const std::string kAcceptCharset;
extern const std::string kAcceptEncoding;
extern const std::string kAcceptLanguage;

// Authentication Credentials
extern const std::string kAuthorization;
extern const std::string kProxyAuthorization;

// Request Context
extern const std::string kFrom;
extern const std::string kReferer;
extern const std::string kUserAgent;

// Response Header Fields

// Control Data
extern const std::string kAge;
extern const std::string kExpires;
extern const std::string kDate;
extern const std::string kLocation;
extern const std::string kRetryAfter;
extern const std::string kVary;
extern const std::string kWarning;

// Validator Header Fields
extern const std::string kETag;
extern const std::string kLastModified;

// Authentication Challenges
extern const std::string kWWWAuthenticate;
extern const std::string kProxyAuthenticate;

// Response Context
extern const std::string kAcceptRanges;
extern const std::string kAllow;
extern const std::string kServer;

// Extra headers
extern const std::string kConnection;

// Tracing headers

extern const std::string kXYaSpanId;
extern const std::string kXYaTraceId;
extern const std::string kXYaRequestId;

}  // namespace headers
}  // namespace http
