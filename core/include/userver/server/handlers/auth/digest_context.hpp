#pragma once

/// @file userver/server/handlers/auth/digest_context.hpp
/// @brief Context structures for Digest Authentication

#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {

/// WWW-Authenticate header from server response
/// realm, nonce directives are mandatory
/// domain, opaque, stale, algorithm, qop, auth-param directives are optional
struct DigestContextFromServer {
  std::string realm;
  std::string nonce;
  std::string algorithm;
  bool stale{false};
  std::string authparam;
  std::string qop;
};

/// Authorization header from client request
/// username, realm, nonce, digest-uri directives response are mandatory
/// algorithm, cnonce, opaque, qop, nc, auth-param directives are optional
struct DigestContextFromClient {
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

/// Function to parse directive map into structure
DigestContextFromClient Parse(
    std::unordered_map<std::string, std::string> directive_mapping);

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END