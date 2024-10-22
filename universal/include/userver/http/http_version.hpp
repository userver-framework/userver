#pragma once

/// @file userver/http/http_version.hpp
/// @brief @copybrief http::HttpVersion

#include <userver/yaml_config/fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace http {

/// @brief HTTP version to use
enum class HttpVersion {
    kDefault,          ///< unspecified version
    k10,               ///< HTTP/1.0 only
    k11,               ///< HTTP/1.1 only
    k2,                ///< HTTP/2 with fallback to HTTP/1.1
    k2Tls,             ///< HTTP/2 over TLS only, otherwise (no TLS or h2) HTTP/1.1
    k2PriorKnowledge,  ///< HTTP/2 only (without Upgrade)
};

HttpVersion Parse(const yaml_config::YamlConfig& value, formats::parse::To<HttpVersion>);

}  // namespace http

USERVER_NAMESPACE_END
