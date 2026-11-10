#pragma once

/// @file userver/crypto/jwt.hpp
/// @brief Parsing and verification of signed JSON Web Tokens
/// @ingroup userver_universal

#include <chrono>
#include <optional>
#include <string>
#include <string_view>

#include <userver/crypto/exception.hpp>
#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

namespace crypto {
class Verifier;
}

/// @brief Signed JWTs in the three-part JWS compact serialization.
namespace crypto::jwt {

/// @brief Decoded JWT data. Values returned by @ref crypto::jwt::Parse are untrusted.
struct Token {
    /// JOSE header, including the signing algorithm and optional key identifier.
    formats::json::Value header;
    /// Claims object. Application-specific authorization claims need application checks.
    formats::json::Value payload;
};

/// Whether verification requires an expiration claim.
enum class ExpirationPolicy {
    kRequired,  ///< Reject tokens without an `exp` claim.
    kOptional,  ///< Allow missing `exp`, but validate it whenever present.
};

/// @brief Claim checks performed by @ref crypto::jwt::Verify after signature verification.
struct VerificationOptions {
    /// Expected `iss`. When set, the claim must be present and match exactly.
    std::optional<std::string> issuer{};
    /// Expected `aud`. Must match the string claim or one element of its string array.
    std::optional<std::string> audience{};
    /// Expected `sub`. When set, the claim must be present and match exactly.
    std::optional<std::string> subject{};
    /// Expected `typ` header. When set, it must be present and match exactly.
    std::optional<std::string> type{};
    /// Nonnegative clock skew allowance for `exp`, `nbf`, and `iat`.
    std::chrono::seconds leeway{0};
    /// Expiration is required by default for authorization tokens.
    ExpirationPolicy expiration{ExpirationPolicy::kRequired};
};

/// @brief Decodes a compact signed JWT without verifying its signature or claims.
/// @warning Do not authorize requests using this result. Use @ref crypto::jwt::Verify.
/// The result owns its data and does not reference @a token.
/// Requires three nonempty, canonical, unpadded base64url parts, UTF-8 JSON objects,
/// unique JSON member names, and a nonempty string `alg`. Nested JWTs, critical
/// extensions, and the `b64` header extension are unsupported and rejected.
/// @throws crypto::VerificationError if the token is malformed or uses unsupported extensions.
Token Parse(std::string_view token);

/// @brief Parses a JWT, verifies its signature, and validates its registered claims.
/// @a verifier must use a trusted key and the algorithm expected by the application.
/// Supported algorithms are HS256/384/512, RS256/384/512, PS256/384/512, and ES256/384/512.
/// The `alg` header must exactly match @ref crypto::NamedAlgo::Name; `none` is rejected.
/// Keys and algorithms are never selected from token-provided URLs or key material.
///
/// Uses @ref utils::datetime::Now. Checks `now < exp + leeway`, `now + leeway >= nbf`,
/// and `now + leeway >= iat` for present claims, accepting fractional NumericDates.
/// Registered string and audience claims are type-checked even without expected values.
/// Missing `nbf` and `iat` are allowed; missing `exp` follows @a options.
/// The result owns its data. Roles, scopes, revocation, and other application policies
/// must be checked by the caller after this function succeeds.
/// @throws crypto::VerificationError if parsing, signature verification, or claim checks fail.
/// @throws std::invalid_argument if @a options contains a negative leeway.
Token Verify(std::string_view token, const crypto::Verifier& verifier, const VerificationOptions& options = {});

}  // namespace crypto::jwt

USERVER_NAMESPACE_END
