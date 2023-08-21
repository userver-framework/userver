#pragma once

/// @file userver/server/handlers/auth/digest_directives.hpp
/// @brief Various digest authentication directives

#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth::directives {

constexpr std::string_view kUsername = "username";
constexpr std::string_view kRealm = "realm";
constexpr std::string_view kNonce = "nonce";
constexpr std::string_view kNextNonce = "nextnonce";
constexpr std::string_view kStale = "stale";
constexpr std::string_view kUri = "uri";
constexpr std::string_view kDomain = "domain";
constexpr std::string_view kResponse = "response";
constexpr std::string_view kAlgorithm = "algorithm";
constexpr std::string_view kCnonce = "cnonce";
constexpr std::string_view kOpaque = "opaque";
constexpr std::string_view kQop = "qop";
constexpr std::string_view kNonceCount = "nc";
constexpr std::string_view kAuthParam = "auth-param";

}  // namespace server::handlers::auth::directives

USERVER_NAMESPACE_END