#include <userver/crypto/jwt.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>

#include <fmt/format.h>

#include <userver/crypto/base64.hpp>
#include <userver/crypto/verifiers.hpp>
#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/utils/datetime_light.hpp>
#include <userver/utils/str_icase.hpp>
#include <userver/utils/text_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace crypto::jwt {
namespace {

struct Parts {
    std::string_view header;
    std::string_view payload;
    std::string_view signature;
    std::string_view signing_input;
};

Parts Split(std::string_view token) {
    const auto first = token.find('.');
    if (first == std::string_view::npos) {
        throw VerificationError("JWT must have three parts");
    }
    const auto second = token.find('.', first + 1);
    if (second == std::string_view::npos || token.find('.', second + 1) != std::string_view::npos) {
        throw VerificationError("JWT must have three parts");
    }
    return {
        token.substr(0, first),
        token.substr(first + 1, second - first - 1),
        token.substr(second + 1),
        token.substr(0, second)
    };
}

std::string Decode(std::string_view part) {
    constexpr std::string_view kAlphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";
    if (part.empty() || part.size() % 4 == 1 || part.find_first_not_of(kAlphabet) != std::string_view::npos) {
        throw VerificationError("Invalid JWT base64url encoding");
    }
    std::string encoded{part};
    std::ranges::replace(encoded, '-', '+');
    std::ranges::replace(encoded, '_', '/');
    auto decoded = base64::Base64Decode(encoded);
    if (base64::Base64Encode(decoded, base64::Pad::kWithout) != encoded) {
        throw VerificationError("Noncanonical JWT base64url encoding");
    }
    return decoded;
}

formats::json::Value ParseObject(std::string_view part) {
    const auto decoded = Decode(part);
    if (!utils::text::IsUtf8(decoded)) {
        throw VerificationError("JWT JSON must be UTF-8");
    }
    try {
        auto value = formats::json::FromString(decoded);
        if (!value.IsObject()) {
            throw VerificationError("JWT header and payload must be JSON objects");
        }
        return value;
    } catch (const formats::json::Exception&) {
        throw VerificationError("Invalid JWT JSON object");
    }
}

Token ParseParts(const Parts& parts) {
    Token result{ParseObject(parts.header), ParseObject(parts.payload)};
    const auto algorithm = result.header["alg"];
    if (!algorithm.IsString() || algorithm.As<std::string>().empty()) {
        throw VerificationError("JWT alg must be a nonempty string");
    }
    if (result.header.HasMember("crit") || result.header.HasMember("b64")) {
        throw VerificationError("JWT critical and b64 extensions are unsupported");
    }
    const auto content_type = result.header["cty"];
    if (content_type.IsString()) {
        const auto type = content_type.As<std::string>();
        if (utils::StrIcaseEqual{}(type, "JWT") || utils::StrIcaseEqual{}(type, "application/jwt")) {
            throw VerificationError("Nested JWTs are unsupported");
        }
    }
    return result;
}

void CheckString(
    const formats::json::Value& object,
    std::string_view name,
    const std::optional<std::string>& expected = std::nullopt
) {
    const auto value = object[name];
    if (value.IsMissing() && !expected) {
        return;
    }
    if (!value.IsString() || (expected && value.As<std::string>() != *expected)) {
        throw VerificationError(fmt::format("JWT {} is missing, invalid, or does not match", name));
    }
}

void CheckAudience(const formats::json::Value& payload, const std::optional<std::string>& expected) {
    const auto audience = payload["aud"];
    if (audience.IsMissing() && !expected) {
        return;
    }
    bool matches = false;
    if (audience.IsString()) {
        matches = !expected || audience.As<std::string>() == *expected;
    } else if (audience.IsArray()) {
        for (const auto& value : audience) {
            if (!value.IsString()) {
                throw VerificationError("JWT aud must be a string or an array of strings");
            }
            if (!expected || value.As<std::string>() == *expected) {
                matches = true;
            }
        }
    } else {
        throw VerificationError("JWT aud must be a string or an array of strings");
    }
    if (expected && !matches) {
        throw VerificationError("JWT audience does not match");
    }
}

std::optional<long double> GetNumericDate(const formats::json::Value& payload, std::string_view name) {
    const auto value = payload[name];
    if (value.IsMissing()) {
        return std::nullopt;
    }
    if (value.IsInt64()) {
        return value.As<std::int64_t>();
    }
    if (value.IsUInt64()) {
        return value.As<std::uint64_t>();
    }
    if (value.IsDouble()) {
        const auto number = value.As<double>();
        if (std::isfinite(number)) {
            return number;
        }
    }
    throw VerificationError(fmt::format("JWT {} must be a finite NumericDate", name));
}

void CheckClaims(const Token& token, const VerificationOptions& options) {
    CheckString(token.header, "typ", options.type);
    CheckString(token.header, "cty");
    CheckString(token.header, "kid");
    CheckString(token.payload, "iss", options.issuer);
    CheckString(token.payload, "sub", options.subject);
    CheckString(token.payload, "jti");
    CheckAudience(token.payload, options.audience);

    const auto now = std::chrono::duration<long double>(utils::datetime::Now().time_since_epoch()).count();
    const auto leeway = std::chrono::duration<long double>(options.leeway).count();
    const auto expiration = GetNumericDate(token.payload, "exp");
    if (!expiration && options.expiration == ExpirationPolicy::kRequired) {
        throw VerificationError("JWT exp is required");
    }
    if (expiration && now - leeway >= *expiration) {
        throw VerificationError("JWT has expired");
    }
    const auto not_before = GetNumericDate(token.payload, "nbf");
    if (not_before && now + leeway < *not_before) {
        throw VerificationError("JWT is not yet valid");
    }
    const auto issued_at = GetNumericDate(token.payload, "iat");
    if (issued_at && now + leeway < *issued_at) {
        throw VerificationError("JWT was issued in the future");
    }
}

void CheckAlgorithm(std::string_view algorithm, const crypto::Verifier& verifier, std::size_t signature_size) {
    constexpr std::size_t kEs256SignatureSize = 64;
    constexpr std::size_t kEs384SignatureSize = 96;
    constexpr std::size_t kEs512SignatureSize = 132;
    constexpr std::array<std::string_view, 12> kAlgorithms{
        "HS256",
        "HS384",
        "HS512",
        "RS256",
        "RS384",
        "RS512",
        "PS256",
        "PS384",
        "PS512",
        "ES256",
        "ES384",
        "ES512"
    };
    if (algorithm != verifier.Name() || std::ranges::find(kAlgorithms, algorithm) == kAlgorithms.end()) {
        throw VerificationError("JWT algorithm is unsupported or does not match the verifier");
    }
    if ((algorithm == "ES256" && signature_size != kEs256SignatureSize) ||
        (algorithm == "ES384" && signature_size != kEs384SignatureSize) ||
        (algorithm == "ES512" && signature_size != kEs512SignatureSize))
    {
        throw VerificationError("Invalid JWT ECDSA signature length");
    }
}

}  // namespace

Token Parse(std::string_view token) {
    const auto parts = Split(token);
    auto result = ParseParts(parts);
    Decode(parts.signature);
    return result;
}

Token Verify(std::string_view token, const crypto::Verifier& verifier, const VerificationOptions& options) {
    if (options.leeway < std::chrono::seconds::zero()) {
        throw std::invalid_argument("JWT leeway must be nonnegative");
    }
    const auto parts = Split(token);
    auto result = ParseParts(parts);
    const auto signature = Decode(parts.signature);
    CheckAlgorithm(result.header["alg"].As<std::string>(), verifier, signature.size());
    verifier.Verify({parts.signing_input}, signature);
    CheckClaims(result, options);
    return result;
}

}  // namespace crypto::jwt

USERVER_NAMESPACE_END
