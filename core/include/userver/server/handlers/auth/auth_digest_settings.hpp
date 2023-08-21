#pragma once

/// @file userver/server/handlers/auth/auth_digest_settings.hpp
/// @brief @copybrief server::handlers::auth::AuthDigestSettings

#include <chrono>
#include <optional>
#include <string>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {

/// @brief Struct of directives for digest authentication server settings per
/// RFC 2617
struct AuthDigestSettings {
  /// Algorithm for hashing `nonce` from
  /// https://datatracker.ietf.org/doc/html/rfc2617#section-3.2.1
  std::string algorithm;
  /// `domains` from https://datatracker.ietf.org/doc/html/rfc2617#section-3.2.1
  std::vector<std::string> domains;
  /// `qop-options` from
  /// https://datatracker.ietf.org/doc/html/rfc2617#section-3.2.1
  std::vector<std::string> qops;
  /// If set, the Proxy prefix is inserted into the header of responses
  bool is_proxy;
  /// Enable sessions and session algorithms (MD5-sess, SHA-256-sess and
  /// SHA-512-256-sess)
  bool is_session;
  /// ttl for `nonce` from
  /// https://datatracker.ietf.org/doc/html/rfc2617#section-3.2.1
  std::chrono::milliseconds nonce_ttl;
};

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END
