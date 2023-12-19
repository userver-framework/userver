#pragma once

/// @file userver/server/handlers/auth/digest/context.hpp
/// @brief Context structures for Digest Authentication

#include <string>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth::digest {

/// WWW-Authenticate header from server response
/// realm, nonce directives are mandatory
/// domain, opaque, stale, algorithm, qop, auth-param directives are optional
struct ContextFromServer {
  std::string realm;
  std::string nonce;
  std::string algorithm;
  bool stale{false};
  std::string authparam;
  std::string qop;
  std::string opaque;
};

/// Authorization header from client request
/// username, realm, nonce, digest-uri directives response are mandatory
/// algorithm, cnonce, opaque, qop, nc, auth-param directives are optional
struct ContextFromClient {
  std::string username;
  std::string realm;
  std::string nonce;
  std::string uri;
  std::string response;
  std::string algorithm;
  std::string cnonce;
  std::string opaque;
  std::string qop;
  std::string nc;
  std::string authparam;
};

/// Max number of directives in Authorization header.
/// Must be equal to the number of DigestContextFromClient fields.
inline constexpr std::size_t kMaxClientDirectivesNumber = 11;

/// Number of mandatory directives in Authorization header.
inline constexpr std::size_t kClientMandatoryDirectivesNumber = 5;

}  // namespace server::handlers::auth::digest
USERVER_NAMESPACE_END
