#pragma once

USERVER_NAMESPACE_BEGIN

namespace utils::http {

/// HTTP version to use
enum class HttpVersion {
  kDefault,  ///< unspecified version
  k10,       ///< HTTP/1.0 only
  k11,       ///< HTTP/1.1 only
  k2,        ///< HTTP/2 with fallback to HTTP/1.1
  k2Tls,     ///< HTTP/2 over TLS only, otherwise (no TLS or h2) HTTP/1.1
  k2PriorKnowledge,  ///< HTTP/2 only (without Upgrade)
};

}  // namespace utils::http

USERVER_NAMESPACE_END
