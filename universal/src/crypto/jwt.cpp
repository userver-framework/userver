#include <userver/crypto/jwt.hpp>

#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <stdexcept>
#include <utility>

#include <fmt/format.h>

#include <userver/crypto/base64.hpp>
#include <userver/crypto/exception.hpp>
#include <userver/crypto/signers.hpp>
#include <userver/crypto/verifiers.hpp>
#include <userver/formats/common/items.hpp>
#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/string_builder.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/utils/algo.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/datetime_light.hpp>
#include <userver/utils/expected.hpp>
#include <userver/utils/str_icase.hpp>
#include <userver/utils/string_literal.hpp>
#include <userver/utils/text_light.hpp>
#include <userver/utils/trivial_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace crypto::jwt {

namespace {

constexpr utils::TrivialSet kReservedHeaderNames = [](auto selector) {
    return selector().Case("alg").Case("typ").Case("cty").Case("kid").Case("crit").Case("b64");
};

constexpr utils::TrivialSet kSupportedAlgorithms = [](auto selector) {
    return selector()
        .Case("HS256")
        .Case("HS384")
        .Case("HS512")
        .Case("RS256")
        .Case("RS384")
        .Case("RS512")
        .Case("PS256")
        .Case("PS384")
        .Case("PS512")
        .Case("ES256")
        .Case("ES384")
        .Case("ES512");
};

template <typename Error>
void CheckAlgorithm(std::string_view algorithm) {
    if (!kSupportedAlgorithms.Contains(algorithm)) {
        throw Error("Unsupported JWT algorithm");
    }
}

struct PartsView {
    std::string_view header;
    std::string_view payload;
    std::string_view signature;
    std::string_view signing_input;
};

utils::expected<PartsView, utils::StringLiteral> SplitCompactToken(std::string_view token) {
    constexpr utils::StringLiteral kInvalidStructure = "JWT must have three nonempty parts";
    const auto first = token.find('.');
    if (first == std::string_view::npos || first == 0) {
        return utils::unexpected{kInvalidStructure};
    }
    const auto second = token.find('.', first + 1);
    if (second == std::string_view::npos || second == first + 1 || second + 1 == token.size() ||
        token.find('.', second + 1) != std::string_view::npos)
    {
        return utils::unexpected{kInvalidStructure};
    }
    return PartsView{
        token.substr(0, first),
        token.substr(first + 1, second - first - 1),
        token.substr(second + 1),
        token.substr(0, second)
    };
}

PartsView SplitSignedToken(std::string_view token) {
    const auto parts = SplitCompactToken(token);
    if (!parts) {
        throw VerificationError(parts.error());
    }
    return *parts;
}

std::string ReadCompactPart(std::string_view part) {
    // Compact JWS requires unpadded base64url without line breaks and with zero unused bits.
    constexpr std::string_view kFourZeroBits = "AQgw";
    constexpr std::string_view kTwoZeroBits = "AEIMQUYcgkosw048";
    const auto remainder = part.size() % 4;
    if (part.find_first_of("=\r\n") != std::string_view::npos ||
        (remainder == 2 && kFourZeroBits.find(part.back()) == std::string_view::npos) ||
        (remainder == 3 && kTwoZeroBits.find(part.back()) == std::string_view::npos))
    {
        throw VerificationError("Noncanonical JWT base64url encoding");
    }
    try {
        return base64::Base64UrlDecode(part);
    } catch (const CryptoException&) {
        throw VerificationError("Invalid JWT base64url encoding");
    }
}

formats::json::Value ParseObject(std::string_view decoded) {
    if (!utils::text::IsUtf8(decoded)) {
        throw VerificationError("JWT JSON must be UTF-8");
    }
    try {
        const auto value = formats::json::FromString(decoded);
        value.CheckObject();
        return value;
    } catch (const formats::json::Exception&) {
        throw VerificationError("Invalid JWT JSON object");
    }
}

formats::json::Value GetOptionalStringClaim(const formats::json::Value& payload, std::string_view name) {
    auto value = payload[name];
    if (!value.IsMissing() && !value.IsString()) {
        throw VerificationError(fmt::format("JWT {} must be a string", name));
    }
    return value;
}

std::string_view MediaTypeName(std::string_view value) {
    constexpr std::string_view
        kTokenChars = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!#$%&'*+-.^_`|~";
    const auto base = value.substr(0, value.find(';'));
    const auto last = base.find_last_not_of(" \t");
    if (last == std::string_view::npos) {
        return {};
    }
    const auto name = base.substr(0, last + 1);
    const auto slash = name.find('/');
    if (slash == std::string_view::npos) {
        return name.find_first_not_of(kTokenChars) == std::string_view::npos ? name : std::string_view{};
    }
    if (slash == 0 || slash + 1 == name.size()) {
        return {};
    }
    const auto type = name.substr(0, slash);
    const auto subtype = name.substr(slash + 1);
    if (type.find_first_not_of(kTokenChars) != std::string_view::npos ||
        subtype.find_first_not_of(kTokenChars) != std::string_view::npos)
    {
        return {};
    }
    return utils::StrIcaseEqual{}(type, "application") ? subtype : name;
}

std::string_view TokenTypeName(std::string_view value) {
    // RFC 7515 section 4.1.9 requires the full type if a parameter contains a slash.
    const auto slash = value.find('/');
    if (slash != std::string_view::npos && slash > value.find(';')) {
        return {};
    }
    return MediaTypeName(value);
}

bool MatchesType(std::string_view actual, std::string_view expected) {
    const auto name = TokenTypeName(actual);
    return !name.empty() && utils::StrIcaseEqual{}(name, TokenTypeName(expected));
}

void ValidateHeader(const JoseHeader& header) {
    if (header.algorithm.empty() || !utils::text::IsAscii(header.algorithm)) {
        throw formats::json::Exception("JWT alg must be a nonempty ASCII string");
    }
    if (header.content_type && utils::StrIcaseEqual{}(MediaTypeName(*header.content_type), "JWT")) {
        throw formats::json::Exception("Nested JWTs are unsupported");
    }
}

void CheckAudience(const formats::json::Value& payload, const std::unordered_set<std::string>* expected) {
    const auto audience = payload["aud"];
    if (audience.IsMissing()) {
        if (expected && !expected->empty()) {
            throw VerificationError("JWT audience is missing");
        }
        return;
    }
    bool matches = false;
    const auto check_recipient = [&](const formats::json::Value& recipient) {
        if (!recipient.IsString()) {
            throw VerificationError("JWT aud must be a string or an array of strings");
        }
        if (expected && !matches) {
            matches = expected->contains(recipient.As<std::string>());
        }
    };
    if (audience.IsArray()) {
        for (const auto& recipient : audience) {
            check_recipient(recipient);
        }
    } else {
        check_recipient(audience);
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

void CheckRegisteredClaims(const formats::json::Value& payload, const VerificationOptions* options = nullptr) {
    CheckAudience(payload, options ? &options->audiences : nullptr);
    GetOptionalStringClaim(payload, "jti");
    const auto issuer = GetOptionalStringClaim(payload, "iss");
    const auto subject = GetOptionalStringClaim(payload, "sub");
    const auto expiration = GetNumericDate(payload, "exp");
    const auto not_before = GetNumericDate(payload, "nbf");
    const auto issued_at = GetNumericDate(payload, "iat");
    if (!options) {
        return;
    }
    if (options->issuer && issuer.As<std::optional<std::string>>() != options->issuer) {
        throw VerificationError("JWT issuer is missing or does not match");
    }
    if (options->subject && subject.As<std::optional<std::string>>() != options->subject) {
        throw VerificationError("JWT subject is missing or does not match");
    }

    const auto now = std::chrono::duration<long double>(utils::datetime::Now().time_since_epoch()).count();
    const auto leeway = std::chrono::duration<long double>(options->leeway).count();
    if (!expiration && options->expiration == ExpirationPolicy::kRequired) {
        throw VerificationError("JWT exp is required");
    }
    if (expiration && now - leeway >= *expiration) {
        throw VerificationError("JWT has expired");
    }
    if (not_before && now + leeway < *not_before) {
        throw VerificationError("JWT is not yet valid");
    }
    if (issued_at && now + leeway < *issued_at) {
        throw VerificationError("JWT was issued in the future");
    }
}

JoseHeader ParseCompactHeader(std::string_view encoded) {
    const auto value = ParseObject(ReadCompactPart(encoded));
    try {
        return value.As<JoseHeader>();
    } catch (const formats::json::Exception&) {
        throw VerificationError("Invalid JWT header");
    }
}

std::string MakeCompactToken(std::string_view header, std::string_view payload, const Signer& signer) {
    const auto encoded_header = base64::Base64UrlEncode(header, base64::Pad::kWithout);
    const auto encoded_payload = base64::Base64UrlEncode(payload, base64::Pad::kWithout);
    const auto signature = signer.Sign({encoded_header, ".", encoded_payload});
    const auto encoded_signature = base64::Base64UrlEncode(signature, base64::Pad::kWithout);
    return utils::StrCat(encoded_header, ".", encoded_payload, ".", encoded_signature);
}

std::string MakeTokenWithHeader(
    std::initializer_list<std::pair<std::string_view, std::string_view>> fields,
    std::string_view payload,
    const Signer& signer
) {
    formats::json::StringBuilder header;
    {
        const formats::json::StringBuilder::ObjectGuard guard{header};
        for (const auto& [name, value] : fields) {
            header.Key(name);
            // RapidJSON requires a non-null pointer even for an empty string.
            header.WriteString(value.empty() ? "" : value);
        }
    }
    return MakeCompactToken(header.GetStringView(), payload, signer);
}

}  // namespace

JoseHeader Parse(const formats::json::Value& value, formats::parse::To<JoseHeader>) {
    JoseHeader header{
        .algorithm = value["alg"].As<std::string>(),
        .type = value["typ"].As<std::optional<std::string>>(),
        .content_type = value["cty"].As<std::optional<std::string>>(),
        .key_id = value["kid"].As<std::optional<std::string>>(),
    };
    ValidateHeader(header);
    for (auto [name, member] : formats::common::Items(value)) {
        if (!kReservedHeaderNames.Contains(name)) {
            header.extensions.emplace(std::move(name), member);
        } else if (name == "crit" || name == "b64") {
            throw formats::json::ParseException("JWT critical and b64 extensions are unsupported");
        } else if (member.IsNull()) {
            throw formats::json::ParseException(fmt::format("JWT {} must be a string", name));
        }
    }
    return header;
}

formats::json::Value Serialize(const JoseHeader& header, formats::serialize::To<formats::json::Value>) {
    ValidateHeader(header);
    formats::json::ValueBuilder value;
    value["alg"] = header.algorithm;
    if (header.type) {
        value["typ"] = *header.type;
    }
    if (header.content_type) {
        value["cty"] = *header.content_type;
    }
    if (header.key_id) {
        value["kid"] = *header.key_id;
    }
    for (const auto& [name, member] : header.extensions) {
        if (kReservedHeaderNames.Contains(name)) {
            throw formats::json::Exception("JWT extension conflicts with a reserved header parameter");
        }
        value.EmplaceNocheck(name, member);
    }
    return value.ExtractValue();
}

std::string MakeToken(const formats::json::Value& payload, const crypto::Signer& signer) {
    CheckAlgorithm<SignError>(signer.Name());
    if (!payload.IsObject()) {
        throw SignError("JWT payload must be a JSON object");
    }
    try {
        CheckRegisteredClaims(payload);
    } catch (const VerificationError& error) {
        throw SignError(error.what());
    } catch (const formats::json::Exception&) {
        throw SignError("Invalid JWT claims object");
    }
    const auto claims = formats::json::ToString(payload);
    if (!utils::text::IsUtf8(claims)) {
        throw SignError("JWT claims must be UTF-8");
    }
    return MakeTokenWithHeader({{"alg", signer.Name()}, {"typ", "JWT"}}, claims, signer);
}

std::string MakeToken(std::string_view json_payload, const crypto::Signer& signer) {
    UASSERT_MSG(formats::json::FromString(json_payload).IsRoot(), "payload is not in json format");
    return MakeTokenWithHeader({{"typ", "JWT"}, {"alg", signer.Name()}}, json_payload, signer);
}

std::string MakeToken(std::string_view json_payload, const crypto::Signer& signer, std::string_view key_id) {
    UASSERT_MSG(formats::json::FromString(json_payload).IsRoot(), "payload is not in json format");
    return MakeTokenWithHeader({{"typ", "JWT"}, {"alg", signer.Name()}, {"kid", key_id}}, json_payload, signer);
}

JoseHeader UnverifiedParseJoseHeader(std::string_view token) {
    return ParseCompactHeader(SplitSignedToken(token).header);
}

Token Verify(std::string_view token, const crypto::Verifier& verifier, const VerificationOptions& options) {
    if (options.leeway < std::chrono::seconds::zero()) {
        throw std::invalid_argument("JWT leeway must be nonnegative");
    }
    const auto parts = SplitSignedToken(token);
    auto header = ParseCompactHeader(parts.header);
    if (header.algorithm != verifier.Name()) {
        throw VerificationError("JWT algorithm does not match the verifier");
    }
    CheckAlgorithm<VerificationError>(verifier.Name());
    const auto signature = ReadCompactPart(parts.signature);
    verifier.Verify({parts.signing_input}, signature);
    Token result{std::move(header), ParseObject(ReadCompactPart(parts.payload))};
    if (options.type && (!result.header.type || !MatchesType(*result.header.type, *options.type))) {
        throw VerificationError("JWT typ is missing or does not match");
    }
    CheckRegisteredClaims(result.payload, &options);
    return result;
}

}  // namespace crypto::jwt

USERVER_NAMESPACE_END
