#pragma once

/// @file userver/crypto/jwt.hpp
/// @brief Creation, parsing and verification of signed JSON Web Tokens
/// @ingroup userver_universal
/// @see @ref scripts/docs/en/userver/jwt.md

#include <chrono>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include <userver/crypto/exception.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/formats/serialize/to.hpp>

USERVER_NAMESPACE_BEGIN

namespace crypto {

class Signer;
class Verifier;

}  // namespace crypto

/// @brief Signed JWTs in the three-part JWS compact serialization.
namespace crypto::jwt {

/// @brief JOSE header parameters for a compact signed JWT.
struct JoseHeader {
    std::string algorithm;                      ///< Signing algorithm (`alg`).
    std::optional<std::string> type{};          ///< Token media type (`typ`).
    std::optional<std::string> content_type{};  ///< Payload media type (`cty`).
    std::optional<std::string> key_id{};        ///< Key identifier (`kid`).
    /// Additional noncritical parameters with their original JSON values.
    std::unordered_map<std::string, formats::json::Value> extensions{};
};

/// Parses the JOSE header from a JSON object.
JoseHeader Parse(const formats::json::Value& value, formats::parse::To<JoseHeader>);

/// Serializes a JOSE header to a JSON object, omitting absent parameters.
formats::json::Value Serialize(const JoseHeader& header, formats::serialize::To<formats::json::Value>);

/// @brief Header and claims returned by @ref crypto::jwt::Verify.
struct Token {
    JoseHeader header;
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
    /// Accepted audiences. A present `aud` must match at least one entry.
    /// An empty set accepts only tokens without `aud`; a nonempty set requires `aud`.
    std::unordered_set<std::string> audiences{};
    /// Expected `sub`. When set, the claim must be present and match exactly.
    std::optional<std::string> subject{};
    /// Expected token media type (`typ`). Compares the name case-insensitively and ignores parameters.
    std::optional<std::string> type{};
    /// Nonnegative clock skew allowance for `exp`, `nbf`, and `iat`.
    std::chrono::seconds leeway{0};
    /// Expiration is required by default for authorization tokens.
    ExpirationPolicy expiration{ExpirationPolicy::kRequired};
};

/// @brief Signs the supplied claims with `typ: JWT` and the signer's algorithm.
/// Checks registered claim types before signing.
/// @pre A custom HMAC signer produces the full MAC for its declared algorithm.
/// @pre The caller supplies an HMAC key of at least 32 bytes for HS256, 48 for HS384, or 64 for HS512.
/// @pre Every JSON object in @a payload has unique member names, including nested objects.
/// @throws crypto::SignError for an unsupported algorithm, invalid claims, or a signing failure.
std::string MakeToken(const formats::json::Value& payload, const crypto::Signer& signer);

/// @brief Reads the unverified JOSE header from a compact JWT for key selection.
/// Checks the three-part structure and decodes only the header. The returned algorithm may be unknown.
/// Pass the original token to @ref crypto::jwt::Verify to check the algorithm, signature, and claims.
/// @throws crypto::VerificationError for malformed compact structure, invalid header parameters,
/// unsupported critical or unencoded-payload extensions, or a nested JWT content type.
JoseHeader UnverifiedParseJoseHeader(std::string_view token);

/// @brief Verifies a JWT with the application's trusted key and algorithm, then validates its registered claims.
/// @pre A custom HMAC verifier checks the full MAC, including its length.
/// @pre The caller supplies an HMAC key meeting the minimum length documented in @ref crypto::jwt::MakeToken.
/// @throws crypto::VerificationError if parsing, signature verification, or claim checks fail.
/// @throws std::invalid_argument if @a options contains a negative leeway.
Token Verify(std::string_view token, const crypto::Verifier& verifier, const VerificationOptions& options = {});

}  // namespace crypto::jwt

USERVER_NAMESPACE_END
