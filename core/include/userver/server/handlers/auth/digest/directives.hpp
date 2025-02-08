#pragma once

/// @file userver/server/handlers/auth/digest/directives.hpp
/// @brief Various digest authentication directives

#include <userver/utils/string_literal.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth::digest::directives {

inline constexpr utils::StringLiteral kUsername = "username";
inline constexpr utils::StringLiteral kRealm = "realm";
inline constexpr utils::StringLiteral kNonce = "nonce";
inline constexpr utils::StringLiteral kNextNonce = "nextnonce";
inline constexpr utils::StringLiteral kStale = "stale";
inline constexpr utils::StringLiteral kUri = "uri";
inline constexpr utils::StringLiteral kDomain = "domain";
inline constexpr utils::StringLiteral kResponse = "response";
inline constexpr utils::StringLiteral kAlgorithm = "algorithm";
inline constexpr utils::StringLiteral kCnonce = "cnonce";
inline constexpr utils::StringLiteral kOpaque = "opaque";
inline constexpr utils::StringLiteral kQop = "qop";
inline constexpr utils::StringLiteral kNonceCount = "nc";
inline constexpr utils::StringLiteral kAuthParam = "auth-param";

}  // namespace server::handlers::auth::digest::directives

USERVER_NAMESPACE_END
