#include <http/common_headers.hpp>

namespace http {
namespace headers {

// Headers from rfc7231

// Representation Metadata
const std::string kContentType = "Content-Type";
const std::string kContentEncoding = "Content-Encoding";
const std::string kContentLanguage = "Content-Language";
const std::string kContentLocation = "Content-Location";

// Payload Semantics
const std::string kContentLength = "Content-Length";
const std::string kContentRange = "Content-Range";
const std::string kTrailer = "Trailer";
const std::string kTransferEncoding = "Transfer-Encoding";

// Request Headers - Controls
const std::string kCacheControl = "Cache-Control";
const std::string kExpect = "Expect";
const std::string kHost = "Host";
const std::string kMaxForwards = "Max-Forwards";
const std::string kPragma = "Pragma";
const std::string kRange = "Range";
const std::string kTE = "TE";

// Conditionals
const std::string kIfMatch = "If-Match";
const std::string kIfNoneMatch = "If-None-Match";
const std::string kIfModifiedSince = "If-Modified-Since";
const std::string kIfUnmodifiedSince = "If-Unmodified-Since";
const std::string kIfRange = "If-Range";

// Content Negotiation
const std::string kAccept = "Accept";
const std::string kAcceptCharset = "Accept-Charset";
const std::string kAcceptEncoding = "Accept-Encoding";
const std::string kAcceptLanguage = "Accept-Language";

// Authentication Credentials
const std::string kAuthorization = "Authorization";
const std::string kProxyAuthorization = "Proxy-Authorization";

// Request Context
const std::string kFrom = "From";
const std::string kReferer = "Referer";
const std::string kUserAgent = "User-Agent";

// Response Header Fields

// Control Data
const std::string kAge = "Age";
const std::string kExpires = "Expires";
const std::string kDate = "Date";
const std::string kLocation = "Location";
const std::string kRetryAfter = "Retry-After";
const std::string kVary = "Vary";
const std::string kWarning = "Warning";

// Validator Header Fields
const std::string kETag = "ETag";
const std::string kLastModified = "Last-Modified";

// Authentication Challenges
const std::string kWWWAuthenticate = "WWW-Authenticate";
const std::string kProxyAuthenticate = "Proxy-Authenticate";

// Response Context
const std::string kAcceptRanges = "Accept-Ranges";
const std::string kAllow = "Allow";
const std::string kServer = "Server";

// Extra headers
const std::string kConnection = "Connection";

// Tracing headers

const std::string kXYaRequestId = "X-YaRequestId";
const std::string kXYaTraceId = "X-YaTraceId";
const std::string kXYaSpanId = "X-YaSpanId";

}  // namespace headers
}  // namespace http
