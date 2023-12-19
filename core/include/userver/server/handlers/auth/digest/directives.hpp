#pragma once

/// @file userver/server/handlers/auth/digest/directives.hpp
/// @brief Various digest authentication directives

#include <string>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth::digest::directives {

inline const std::string kUsername = "username";
inline const std::string kRealm = "realm";
inline const std::string kNonce = "nonce";
inline const std::string kNextNonce = "nextnonce";
inline const std::string kStale = "stale";
inline const std::string kUri = "uri";
inline const std::string kDomain = "domain";
inline const std::string kResponse = "response";
inline const std::string kAlgorithm = "algorithm";
inline const std::string kCnonce = "cnonce";
inline const std::string kOpaque = "opaque";
inline const std::string kQop = "qop";
inline const std::string kNonceCount = "nc";
inline const std::string kAuthParam = "auth-param";

}  // namespace server::handlers::auth::digest::directives

USERVER_NAMESPACE_END
