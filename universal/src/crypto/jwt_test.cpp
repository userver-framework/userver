#include <userver/crypto/jwt.hpp>

#include <chrono>
#include <initializer_list>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

#include <fmt/format.h>
#include <gtest/gtest.h>

#include <userver/crypto/base64.hpp>
#include <userver/crypto/signers.hpp>
#include <userver/crypto/verifiers.hpp>
#include <userver/formats/json/exception.hpp>
#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/utest/assert_macros.hpp>
#include <userver/utils/datetime_light.hpp>
#include <userver/utils/mock_now.hpp>

#include "jwt_test_data.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

using crypto::VerificationError;
namespace jwt = crypto::jwt;
using namespace std::chrono_literals;

std::string Encode(std::string_view value) {
    return crypto::base64::Base64UrlEncode(value, crypto::base64::Pad::kWithout);
}

std::string SignInput(std::string_view input, std::string_view key = kHmacKey) {
    const auto signature = crypto::SignerHs256{std::string{key}}.Sign({input});
    return fmt::format("{}.{}", input, Encode(signature));
}

std::string SignRawJson(
    std::string_view payload,
    std::string_view header = R"({"alg":"HS256"})",
    std::string_view key = kHmacKey
) {
    return SignInput(fmt::format("{}.{}", Encode(header), Encode(payload)), key);
}

class CryptoJwt : public testing::Test {
protected:
    void SetUp() override { utils::datetime::MockNowSet(std::chrono::system_clock::time_point{1000s}); }

    void TearDown() override { utils::datetime::MockNowUnset(); }

    const crypto::VerifierHs256 verifier{std::string{kHmacKey}};
};

template <typename Verifier>
std::unique_ptr<crypto::Verifier> MakeVerifier(std::string_view key) {
    return std::make_unique<Verifier>(std::string{key});
}

struct AlgorithmCase {
    std::string algorithm;
    std::string_view token;
    std::string_view key;
    std::unique_ptr<crypto::Verifier> (*make_verifier)(std::string_view);
};

class CryptoJwtAlgorithm : public CryptoJwt, public testing::WithParamInterface<AlgorithmCase> {};

TEST_P(CryptoJwtAlgorithm, IndependentSignature) {
    const auto& test = GetParam();
    const auto signature_verifier = test.make_verifier(test.key);
    const auto
        verified = jwt::Verify(test.token, *signature_verifier, {.issuer = "issuer", .audiences = {"my-service"}});
    EXPECT_EQ(verified.header.algorithm, test.algorithm);
    EXPECT_EQ(verified.payload["sub"].As<std::string>(), "user-42");

    std::string tampered{test.token};
    const auto signature_start = tampered.rfind('.') + 1;
    tampered[signature_start] = tampered[signature_start] == 'A' ? 'B' : 'A';
    EXPECT_THROW(jwt::Verify(tampered, *signature_verifier, {.audiences = {"my-service"}}), VerificationError);
}

INSTANTIATE_TEST_SUITE_P(
    Algorithms,
    CryptoJwtAlgorithm,
    testing::Values(
        AlgorithmCase{"HS256", kHs256Token, kHmacKey, MakeVerifier<crypto::VerifierHs256>},
        AlgorithmCase{"HS384", kHs384Token, kHmacKey, MakeVerifier<crypto::VerifierHs384>},
        AlgorithmCase{"HS512", kHs512Token, kHmacKey, MakeVerifier<crypto::VerifierHs512>},
        AlgorithmCase{"RS256", kRs256Token, kRsPublicKey, MakeVerifier<crypto::VerifierRs256>},
        AlgorithmCase{"RS384", kRs384Token, kRsPublicKey, MakeVerifier<crypto::VerifierRs384>},
        AlgorithmCase{"RS512", kRs512Token, kRsPublicKey, MakeVerifier<crypto::VerifierRs512>},
        AlgorithmCase{"PS256", kPs256Token, kRsPublicKey, MakeVerifier<crypto::VerifierPs256>},
        AlgorithmCase{"PS384", kPs384Token, kRsPublicKey, MakeVerifier<crypto::VerifierPs384>},
        AlgorithmCase{"PS512", kPs512Token, kRsPublicKey, MakeVerifier<crypto::VerifierPs512>},
        AlgorithmCase{"ES256", kEs256Token, kEs256PublicKey, MakeVerifier<crypto::VerifierEs256>},
        AlgorithmCase{"ES384", kEs384Token, kEs384PublicKey, MakeVerifier<crypto::VerifierEs384>},
        AlgorithmCase{"ES512", kEs512Token, kEs512PublicKey, MakeVerifier<crypto::VerifierEs512>}
    ),
    [](const testing::TestParamInfo<AlgorithmCase>& info) { return info.param.algorithm; }
);

TEST_F(CryptoJwt, AuthorizationExample) {
    const std::string_view token = kHs256Token;
    /// [jwt verify]
    const auto verified =
        crypto::jwt::Verify(token, verifier, {.issuer = "issuer", .audiences = {"my-service"}, .type = "JWT"});
    const auto subject = verified.payload["sub"].As<std::string>();
    const auto scope = verified.payload["scope"].As<std::string>();
    /// [jwt verify]
    EXPECT_EQ(subject, "user-42");
    EXPECT_EQ(scope, "read");
}

TEST(CryptoJwtJoseHeader, JsonRoundTrip) {
    const auto value = formats::json::MakeObject(
        "alg",
        "private algorithm:v1",
        "cty",
        "application/json",
        "kid",
        "test-key",
        "custom",
        formats::json::MakeArray(42),
        "custom-null",
        nullptr
    );
    const auto header = value.As<jwt::JoseHeader>();
    EXPECT_EQ(header.algorithm, "private algorithm:v1");
    EXPECT_FALSE(header.type.has_value());
    EXPECT_EQ(header.content_type, "application/json");
    EXPECT_EQ(header.key_id, "test-key");
    ASSERT_EQ(header.extensions.size(), 2);
    EXPECT_EQ(header.extensions.at("custom"), value["custom"]);
    EXPECT_TRUE(header.extensions.at("custom-null").IsNull());
    EXPECT_EQ(formats::json::ValueBuilder{header}.ExtractValue(), value);
}

TEST(CryptoJwtJoseHeader, RejectsReservedExtensionWhenSerializing) {
    const jwt::JoseHeader header{.algorithm = "HS256", .extensions = {{"alg", formats::json::Value{}}}};
    EXPECT_THROW(formats::json::ValueBuilder{header}.ExtractValue(), formats::json::Exception);
}

TEST_F(CryptoJwt, IssueToken) {
    const crypto::SignerHs256 signer{std::string{kHmacKey}};
    const std::string user_id = "user-42";
    /// [jwt issue]
    const auto expires_at = utils::datetime::Timestamp(utils::datetime::Now() + std::chrono::minutes{15});
    const auto payload = formats::json::MakeObject("id", user_id, "exp", expires_at);
    const auto token = crypto::jwt::MakeToken(payload, signer);
    /// [jwt issue]
    const auto verified = jwt::Verify(token, verifier, {.type = "JWT"});
    EXPECT_EQ(verified.header.algorithm, "HS256");
    EXPECT_EQ(verified.payload, payload);
}

TEST(CryptoJwtInterop, SignsVerbatimPayloadWithJsonEscapedHeader) {
    class CustomSigner final : public crypto::Signer {
    public:
        using Signer::Signer;

        std::string Sign(std::initializer_list<std::string_view> data) const override {
            return crypto::SignerHs256{std::string{kHmacKey}}.Sign(data);
        }
    };
    using namespace std::string_view_literals;
    const CustomSigner signer{std::string{"custom\"\\\n\0alg"sv}};
    constexpr std::string_view kPayloadBuffer = R"({ "exp":null }!)";
    constexpr auto kPayload = kPayloadBuffer.substr(0, kPayloadBuffer.size() - 1);
    constexpr auto kKeyIdBuffer = "rotating\t\"\x1f\\key!"sv;
    constexpr auto kKeyId = kKeyIdBuffer.substr(0, kKeyIdBuffer.size() - 1);

    const auto header = formats::json::MakeObject("typ", "JWT", "alg", signer.Name());
    EXPECT_EQ(jwt::MakeToken(kPayload, signer), SignRawJson(kPayload, formats::json::ToString(header)));
    const auto header_with_key = formats::json::MakeObject("typ", "JWT", "alg", signer.Name(), "kid", kKeyId);
    EXPECT_EQ(
        jwt::MakeToken(kPayload, signer, kKeyId),
        SignRawJson(kPayload, formats::json::ToString(header_with_key))
    );
}

TEST(CryptoJwtInterop, DistinguishesMissingAndEmptyKeyId) {
    const crypto::SignerNone signer;
    EXPECT_EQ(jwt::MakeToken("null", signer), "eyJ0eXAiOiJKV1QiLCJhbGciOiJub25lIn0.bnVsbA.");
    EXPECT_EQ(
        jwt::MakeToken("null", signer, std::string_view{}),
        "eyJ0eXAiOiJKV1QiLCJhbGciOiJub25lIiwia2lkIjoiIn0.bnVsbA."
    );
}

TEST(CryptoJwtInterop, MatchesRealmediumTokenFormat) {
    const auto payload = formats::json::MakeObject("id", "user-42");
    const jwt::VerificationOptions options{.expiration = jwt::ExpirationPolicy::kOptional};
    const crypto::VerifierHs256 verifier{std::string{kCppJwtRealmediumKey}};
    EXPECT_EQ(jwt::Verify(kCppJwtRealmediumToken, verifier, options).payload, payload);
    UEXPECT_THROW_MSG(
        jwt::Verify(kRealmediumUnauthorizedToken, verifier, options),
        VerificationError,
        "Invalid signature"
    );

    const crypto::SignerHs256 signer{std::string{kCppJwtRealmediumKey}};
    EXPECT_EQ(jwt::MakeToken(payload, signer), kCppJwtRealmediumToken);

    const crypto::VerifierHs256 legacy_verifier{"secret"};
    EXPECT_EQ(jwt::Verify(kLegacyRealmediumToken, legacy_verifier, options).payload, payload);
    EXPECT_EQ(jwt::MakeToken(payload, crypto::SignerHs256{"secret"}), kLegacyRealmediumToken);
}

TEST_F(CryptoJwt, MatchesPublishedCppJwtHs256Token) {
    const auto claims = formats::json::MakeObject("aud", "rift.io", "exp", 1513863371, "sub", "nothing much");
    const crypto::VerifierHs256 upstream_verifier{std::string{kCppJwtUpstreamKey}};
    const auto verified = jwt::Verify(kCppJwtUpstreamToken, upstream_verifier, {.audiences = {"rift.io"}});
    EXPECT_EQ(verified.payload, claims);

    const crypto::SignerHs256 signer{std::string{kCppJwtUpstreamKey}};
    EXPECT_EQ(jwt::MakeToken(claims, signer), kCppJwtUpstreamToken);
}

TEST_F(CryptoJwt, RejectsUnsupportedSigner) {
    EXPECT_THROW(jwt::MakeToken(formats::json::MakeObject(), crypto::SignerNone{}), crypto::SignError);
}

TEST_F(CryptoJwt, RejectsNonObjectPayloadWhenSigning) {
    const crypto::SignerHs256 signer{std::string{kHmacKey}};
    EXPECT_THROW(jwt::MakeToken(formats::json::MakeArray("user-42"), signer), crypto::SignError);
}

TEST(CryptoJwtJoseHeader, ReadsHeaderWithoutDecodingPayloadOrSignature) {
    const auto token = fmt::format("{}.?.?", Encode(R"({"alg":"private-alg","kid":"active"})"));
    const auto header = jwt::UnverifiedParseJoseHeader(token);
    EXPECT_EQ(header.algorithm, "private-alg");
    EXPECT_EQ(header.key_id, "active");
}

/// [jwt key selection]
crypto::jwt::Token VerifyWithKeyId(
    std::string_view token,
    const std::unordered_map<std::string, const crypto::Verifier*>& trusted_keys,
    const crypto::jwt::VerificationOptions& options
) {
    const auto header = crypto::jwt::UnverifiedParseJoseHeader(token);
    if (!header.key_id) {
        throw crypto::VerificationError("Missing JWT key ID");
    }
    const auto key = trusted_keys.find(*header.key_id);
    if (key == trusted_keys.end()) {
        throw crypto::VerificationError("Unknown JWT key ID");
    }
    return crypto::jwt::Verify(token, *key->second, options);
}
/// [jwt key selection]

TEST_F(CryptoJwt, SelectsKeyFromUnverifiedHeader) {
    constexpr std::string_view kClaims = R"({"exp":2000,"sub":"user-42","aud":"my-service"})";
    constexpr std::string_view kCurrentHeader = R"({"alg":"HS256","kid":"current"})";
    constexpr std::string_view kPreviousHeader = R"({"alg":"HS256","kid":"previous"})";
    const std::string previous_key(32, 'p');
    const crypto::VerifierHs256 previous_verifier{previous_key};
    const std::unordered_map<std::string, const crypto::Verifier*>
        trusted_keys{{"current", &verifier}, {"previous", &previous_verifier}};
    const jwt::VerificationOptions options{.audiences = {"my-service"}};

    const auto current = SignRawJson(kClaims, kCurrentHeader);
    const auto previous = SignRawJson(kClaims, kPreviousHeader, previous_key);
    EXPECT_EQ(VerifyWithKeyId(current, trusted_keys, options).payload["sub"].As<std::string>(), "user-42");
    EXPECT_EQ(VerifyWithKeyId(previous, trusted_keys, options).payload["sub"].As<std::string>(), "user-42");
    UEXPECT_THROW_MSG(
        VerifyWithKeyId(current, trusted_keys, {.audiences = {"other-service"}}),
        VerificationError,
        "audience"
    );

    const auto missing = SignRawJson(kClaims);
    const auto unknown = SignRawJson(kClaims, R"({"alg":"HS256","kid":"CURRENT"})");
    UEXPECT_THROW_MSG(VerifyWithKeyId(missing, trusted_keys, options), VerificationError, "Missing JWT key ID");
    UEXPECT_THROW_MSG(VerifyWithKeyId(unknown, trusted_keys, options), VerificationError, "Unknown JWT key ID");

    const auto wrong_current = SignRawJson(kClaims, kCurrentHeader, previous_key);
    const auto wrong_previous = SignRawJson(kClaims, kPreviousHeader);
    UEXPECT_THROW_MSG(VerifyWithKeyId(wrong_current, trusted_keys, options), VerificationError, "Invalid signature");
    UEXPECT_THROW_MSG(VerifyWithKeyId(wrong_previous, trusted_keys, options), VerificationError, "Invalid signature");
}

TEST_F(CryptoJwt, ChecksSignatureBeforeParsingPayload) {
    const auto malformed = SignRawJson("{");
    const crypto::VerifierHs256 wrong_key{std::string(64, 'x')};
    UEXPECT_THROW_MSG(jwt::Verify(malformed, wrong_key), VerificationError, "Invalid signature");
}

TEST_F(CryptoJwt, RejectsTruncatedHmac) {
    const auto start = kHs256Token.rfind('.') + 1;
    auto signature = crypto::base64::Base64UrlDecode(kHs256Token.substr(start));
    signature.pop_back();
    const auto token = fmt::format("{}{}", kHs256Token.substr(0, start), Encode(signature));
    UEXPECT_THROW_MSG(
        jwt::Verify(token, verifier, {.audiences = {"my-service"}}),
        VerificationError,
        "Invalid signature"
    );
}

TEST_F(CryptoJwt, RejectsModifiedHeader) {
    const auto first = kHs256Token.find('.');
    const auto
        modified = fmt::format("{}.{}", Encode(R"({"alg":"HS256","kid":"changed"})"), kHs256Token.substr(first + 1));
    EXPECT_THROW(jwt::Verify(modified, verifier, {.audiences = {"my-service"}}), VerificationError);
}

TEST_F(CryptoJwt, RejectsModifiedPayload) {
    const auto first = kHs256Token.find('.');
    const auto second = kHs256Token.rfind('.');
    const auto modified = fmt::format(
        "{}.{}.{}",
        kHs256Token.substr(0, first),
        Encode(R"({"exp":2000,"sub":"changed"})"),
        kHs256Token.substr(second + 1)
    );
    EXPECT_THROW(jwt::Verify(modified, verifier), VerificationError);
}

TEST_F(CryptoJwt, AlgorithmCannotBeSelectedByToken) {
    for (const auto algorithm : {"none", "HS1", "RS256", "HS384", "hs256", "unknown"}) {
        SCOPED_TRACE(algorithm);
        const auto token = SignRawJson(R"({"exp":2000})", fmt::format(R"({{"alg":"{}"}})", algorithm));
        EXPECT_THROW(jwt::Verify(token, verifier), VerificationError);
    }
    EXPECT_THROW(
        jwt::Verify(SignRawJson(R"({"exp":2000})", R"({"alg":"none"})"), crypto::VerifierNone{}),
        VerificationError
    );

    const auto confused = fmt::format("{}.{}", Encode(R"({"alg":"HS256"})"), Encode(R"({"exp":2000})"));
    const auto forged =
        fmt::format("{}.{}", confused, Encode(crypto::SignerHs256{std::string{kRsPublicKey}}.Sign({confused})));
    EXPECT_THROW(jwt::Verify(forged, crypto::VerifierRs256{kRsPublicKey}), VerificationError);
}

struct NamedInput {
    std::string_view name;
    std::string_view value;
};

std::string CaseName(const testing::TestParamInfo<NamedInput>& info) { return std::string{info.param.name}; }

class CryptoJwtCompact : public testing::TestWithParam<NamedInput> {};

TEST_P(CryptoJwtCompact, RejectsMalformedEnvelope) {
    UEXPECT_THROW_MSG(jwt::UnverifiedParseJoseHeader(GetParam().value), VerificationError, "three nonempty parts");
}

INSTANTIATE_TEST_SUITE_P(
    Structure,
    CryptoJwtCompact,
    testing::Values(
        NamedInput{"MissingSegments", "eyJhbGciOiJIUzI1NiJ9"},
        NamedInput{"MissingSignatureSegment", "eyJhbGciOiJIUzI1NiJ9.e30"},
        NamedInput{"EmptyHeader", ".e30.AA"},
        NamedInput{"EmptyPayload", "eyJhbGciOiJIUzI1NiJ9..AA"},
        NamedInput{"EmptySignature", "eyJhbGciOiJIUzI1NiJ9.e30."}
    ),
    CaseName
);

TEST(CryptoJwtJoseHeader, RejectsExtraSegment) {
    const auto token = fmt::format("{}.AA", kHs256Token);
    UEXPECT_THROW_MSG(jwt::UnverifiedParseJoseHeader(token), VerificationError, "three nonempty parts");
}

class CryptoJwtSignatureEncoding : public CryptoJwt, public testing::WithParamInterface<NamedInput> {};

TEST_P(CryptoJwtSignatureEncoding, RejectsBeforeSignatureComparison) {
    const auto signed_input = kHs256Token.substr(0, kHs256Token.rfind('.'));
    const auto token = fmt::format("{}.{}", signed_input, GetParam().value);
    UEXPECT_THROW_MSG(jwt::Verify(token, verifier, {.audiences = {"my-service"}}), VerificationError, "base64url");
}

INSTANTIATE_TEST_SUITE_P(
    Encoding,
    CryptoJwtSignatureEncoding,
    testing::Values(
        NamedInput{"Padded", "AA=="},
        NamedInput{"InvalidLength", "A"},
        NamedInput{"StandardAlphabetPlus", "+A"},
        NamedInput{"StandardAlphabetSlash", "/A"},
        NamedInput{"EmbeddedSpace", "A A"},
        NamedInput{"EmbeddedNewline", "A\nA"},
        NamedInput{"NonzeroUnusedFourBits", "AB"},
        NamedInput{"NonzeroUnusedTwoBits", "AAB"}
    ),
    CaseName
);

TEST_F(CryptoJwt, RejectsPaddingInSignedSegments) {
    const auto header = Encode(R"({"alg": "HS256" })");
    const auto payload = Encode(R"({"exp":200000})");
    for (const auto& input : {fmt::format("{}=.{}", header, payload), fmt::format("{}.{}=", header, payload)}) {
        SCOPED_TRACE(input);
        UEXPECT_THROW_MSG(jwt::Verify(SignInput(input), verifier), VerificationError, "base64url");
    }
}

class CryptoJwtPayloadJson : public CryptoJwt, public testing::WithParamInterface<NamedInput> {};

TEST_P(CryptoJwtPayloadJson, RejectsInvalidObject) {
    const auto token = SignRawJson(GetParam().value);
    const jwt::VerificationOptions options{.expiration = jwt::ExpirationPolicy::kOptional};
    UEXPECT_THROW_MSG(jwt::Verify(token, verifier, options), VerificationError, "JWT JSON");
}

INSTANTIATE_TEST_SUITE_P(
    Json,
    CryptoJwtPayloadJson,
    testing::Values(
        NamedInput{"Null", "null"},
        NamedInput{"Array", "[]"},
        NamedInput{"Number", "42"},
        NamedInput{"WhitespaceOnly", " "},
        NamedInput{"UnterminatedObject", "{"},
        NamedInput{"TrailingObject", "{}{}"},
        NamedInput{"DuplicateMember", R"({"sub":"first","sub":"second"})"},
        NamedInput{"NestedDuplicateMember", R"({"a":{"b":1,"b":2}})"},
        NamedInput{"UnpairedSurrogate", R"({"a":"\ud800"})"},
        NamedInput{"InvalidUtf8", "{\"sub\":\"\xff\"}"},
        NamedInput{"NumberOutOfRange", R"({"value":1e400})"}
    ),
    CaseName
);

class CryptoJwtHeader : public testing::TestWithParam<NamedInput> {};

TEST_P(CryptoJwtHeader, RejectsInvalidHeader) {
    const auto token = fmt::format("{}.e30.AA", Encode(GetParam().value));
    EXPECT_THROW(jwt::UnverifiedParseJoseHeader(token), VerificationError);
}

INSTANTIATE_TEST_SUITE_P(
    Jose,
    CryptoJwtHeader,
    testing::Values(
        NamedInput{"UnterminatedObject", "{"},
        NamedInput{"MissingAlgorithm", R"({})"},
        NamedInput{"DuplicateAlgorithm", R"({"alg":"HS256","alg":"HS256"})"},
        NamedInput{"NullAlgorithm", R"({"alg":null})"},
        NamedInput{"NumericAlgorithm", R"({"alg":1})"},
        NamedInput{"EmptyAlgorithm", R"({"alg":""})"},
        NamedInput{"NonAsciiAlgorithm", R"({"alg":"H\u00e9"})"},
        NamedInput{"NumericType", R"({"alg":"HS256","typ":42})"},
        NamedInput{"NullKeyId", R"({"alg":"HS256","kid":null})"},
        NamedInput{"NullContentType", R"({"alg":"HS256","cty":null})"},
        NamedInput{"CriticalExtension", R"({"alg":"HS256","crit":["custom"],"custom":true})"},
        NamedInput{"EmptyCriticalExtension", R"({"alg":"HS256","crit":[]})"},
        NamedInput{"NullCriticalExtension", R"({"alg":"HS256","crit":null})"},
        NamedInput{"UnencodedPayload", R"({"alg":"HS256","b64":false})"},
        NamedInput{"Base64Extension", R"({"alg":"HS256","b64":true})"},
        NamedInput{"NestedToken", R"({"alg":"HS256","cty":"jwt"})"},
        NamedInput{"NestedMediaType", R"({"alg":"HS256","cty":"application/JWT"})"},
        NamedInput{"NestedTokenWithParameter", R"({"alg":"HS256","cty":"JWT; a=b"})"},
        NamedInput{"NestedMediaTypeWithSlashInParameter", R"({"alg":"HS256","cty":"application/jwt; x=/"})"}
    ),
    CaseName
);

TEST_F(CryptoJwt, ExpirationAndClockSkewBoundaries) {
    const auto expired = SignRawJson(R"({"exp":1000})");
    EXPECT_THROW(jwt::Verify(expired, verifier), VerificationError);
    EXPECT_NO_THROW(jwt::Verify(expired, verifier, {.leeway = 1s}));
    EXPECT_THROW(jwt::Verify(SignRawJson(R"({"exp":999})"), verifier, {.leeway = 1s}), VerificationError);
    EXPECT_NO_THROW(jwt::Verify(SignRawJson(R"({"exp":1000.5})"), verifier));
    utils::datetime::MockNowSet(std::chrono::system_clock::time_point{1000s + 500ms});
    EXPECT_THROW(jwt::Verify(SignRawJson(R"({"exp":1000.5})"), verifier), VerificationError);
}

TEST_F(CryptoJwt, NotBeforeAndIssuedAtBoundaries) {
    for (const auto claim : {"nbf", "iat"}) {
        SCOPED_TRACE(claim);
        EXPECT_NO_THROW(jwt::Verify(SignRawJson(fmt::format(R"({{"exp":2000,"{}":1000}})", claim)), verifier));
        const auto future = SignRawJson(fmt::format(R"({{"exp":2000,"{}":1001}})", claim));
        EXPECT_THROW(jwt::Verify(future, verifier), VerificationError);
        EXPECT_NO_THROW(jwt::Verify(future, verifier, {.leeway = 1s}));
    }
}

TEST_F(CryptoJwt, AllowsMissingExpirationOnlyWhenOptional) {
    const auto token = SignRawJson("{}");
    EXPECT_THROW(jwt::Verify(token, verifier), VerificationError);
    EXPECT_NO_THROW(jwt::Verify(token, verifier, {.expiration = jwt::ExpirationPolicy::kOptional}));
}

TEST_F(CryptoJwt, OptionalExpirationStillRejectsExpiredToken) {
    EXPECT_THROW(
        jwt::Verify(SignRawJson(R"({"exp":999})"), verifier, {.expiration = jwt::ExpirationPolicy::kOptional}),
        VerificationError
    );
}

TEST_F(CryptoJwt, RejectsNegativeLeeway) {
    EXPECT_THROW(jwt::Verify(kHs256Token, verifier, {.leeway = -1s}), std::invalid_argument);
}

TEST(CryptoJwtClaims, RejectsInvalidClaimsWhenSigning) {
    const crypto::SignerHs256 signer{std::string{kHmacKey}};
    UEXPECT_THROW_MSG(jwt::MakeToken(formats::json::MakeObject("exp", nullptr), signer), crypto::SignError, "exp");
    UEXPECT_THROW_MSG(
        jwt::MakeToken(formats::json::MakeObject("aud", formats::json::MakeObject()), signer),
        crypto::SignError,
        "aud"
    );
}

class CryptoJwtClaimType : public CryptoJwt, public testing::WithParamInterface<NamedInput> {};

TEST_P(CryptoJwtClaimType, RejectsInvalidClaimType) {
    EXPECT_THROW(
        jwt::Verify(SignRawJson(GetParam().value), verifier, {.expiration = jwt::ExpirationPolicy::kOptional}),
        VerificationError
    );
}

INSTANTIATE_TEST_SUITE_P(
    Claims,
    CryptoJwtClaimType,
    testing::Values(
        NamedInput{"NullExpiration", R"({"exp":null})"},
        NamedInput{"BooleanNotBefore", R"({"nbf":true})"},
        NamedInput{"StringIssuedAt", R"({"iat":"1000"})"},
        NamedInput{"NullIssuer", R"({"iss":null})"},
        NamedInput{"NumericSubject", R"({"sub":123})"},
        NamedInput{"BooleanIdentifier", R"({"jti":true})"}
    ),
    CaseName
);

TEST_F(CryptoJwt, AcceptsExtremeNumericDates) {
    EXPECT_NO_THROW(
        jwt::Verify(SignRawJson(R"({"exp":18446744073709551615,"nbf":-9223372036854775808,"iat":-1e100})"), verifier)
    );
}

TEST_F(CryptoJwt, RejectsLargeNegativeExpiration) {
    EXPECT_THROW(jwt::Verify(SignRawJson(R"({"exp":-9223372036854775808})"), verifier), VerificationError);
}

struct ExpectedClaim {
    std::string_view name;
    jwt::VerificationOptions options;
};

class CryptoJwtExpectedClaim : public CryptoJwt, public testing::WithParamInterface<ExpectedClaim> {};

TEST_P(CryptoJwtExpectedClaim, RejectsMissingClaim) {
    EXPECT_THROW(jwt::Verify(SignRawJson(R"({"exp":2000})"), verifier, GetParam().options), VerificationError);
}

TEST_P(CryptoJwtExpectedClaim, RejectsMismatchedClaim) {
    auto options = GetParam().options;
    options.audiences.emplace("my-service");
    EXPECT_THROW(jwt::Verify(kHs256Token, verifier, options), VerificationError);
}

INSTANTIATE_TEST_SUITE_P(
    Expectations,
    CryptoJwtExpectedClaim,
    testing::Values(
        ExpectedClaim{"Issuer", {.issuer = "another-issuer"}},
        ExpectedClaim{"Subject", {.subject = "another-user"}},
        ExpectedClaim{"Type", {.type = "at+jwt"}}
    ),
    [](const testing::TestParamInfo<ExpectedClaim>& info) { return std::string{info.param.name}; }
);

TEST_F(CryptoJwt, RejectsPresentAudienceWithoutAnExpectedRecipient) {
    for (const auto claims :
         {R"({"exp":2000,"aud":"other"})", R"({"exp":2000,"aud":["other"]})", R"({"exp":2000,"aud":[]})"})
    {
        SCOPED_TRACE(claims);
        const auto token = SignRawJson(claims);
        UEXPECT_THROW_MSG(jwt::Verify(token, verifier), VerificationError, "audience");
    }
    const auto no_audience = SignRawJson(R"({"exp":2000})");
    EXPECT_NO_THROW(jwt::Verify(no_audience, verifier));
    UEXPECT_THROW_MSG(jwt::Verify(no_audience, verifier, {.audiences = {"my-service"}}), VerificationError, "audience");
}

TEST_F(CryptoJwt, MatchesStringIdentifiersExactly) {
    const auto claims = formats::json::MakeObject("iss", "Issuer ID:42", "sub", "user:%2X", "aud", "service:my app");
    const auto token = jwt::MakeToken(claims, crypto::SignerHs256{std::string{kHmacKey}});
    jwt::VerificationOptions options{
        .issuer = "Issuer ID:42",
        .audiences = {"service:my app"},
        .subject = "user:%2X",
        .expiration = jwt::ExpirationPolicy::kOptional,
    };
    EXPECT_EQ(jwt::Verify(token, verifier, options).payload, claims);
    options.issuer = "issuer ID:42";
    EXPECT_THROW(jwt::Verify(token, verifier, options), VerificationError);
}

TEST_F(CryptoJwt, IssuesTokensBeforeTheirValidityWindow) {
    const auto claims = formats::json::MakeObject("nbf", 1001, "exp", 2000);
    const auto token = jwt::MakeToken(claims, crypto::SignerHs256{std::string{kHmacKey}});
    EXPECT_THROW(jwt::Verify(token, verifier), VerificationError);
}

TEST_F(CryptoJwt, MatchesAnyConfiguredAudience) {
    const jwt::VerificationOptions options{.audiences = {"another-service", "my-service"}};
    const auto string_audience = SignRawJson(R"({"exp":2000,"aud":"my-service"})");
    EXPECT_NO_THROW(jwt::Verify(string_audience, verifier, options));

    const auto array_audience = SignRawJson(R"({"exp":2000,"aud":["first-service","my-service"]})");
    EXPECT_NO_THROW(jwt::Verify(array_audience, verifier, options));
    UEXPECT_THROW_MSG(
        jwt::Verify(array_audience, verifier, {.audiences = {"MY-SERVICE"}}),
        VerificationError,
        "audience"
    );
}

TEST_F(CryptoJwt, RejectsEmptyAudienceWhenRequired) {
    const auto token = SignRawJson(R"({"exp":2000,"aud":[]})");
    EXPECT_THROW(jwt::Verify(token, verifier, {.audiences = {"my-service"}}), VerificationError);
}

TEST_F(CryptoJwt, ValidatesAudienceTypeAndEveryArrayElement) {
    for (const auto claims : {R"({"exp":2000,"aud":{}})", R"({"exp":2000,"aud":["my-service",null]})"}) {
        SCOPED_TRACE(claims);
        UEXPECT_THROW_MSG(
            jwt::Verify(SignRawJson(claims), verifier, {.audiences = {"my-service"}}),
            VerificationError,
            "JWT aud must be a string"
        );
    }
}

TEST_F(CryptoJwt, RecognizesApplicationMediaType) {
    EXPECT_NO_THROW(jwt::Verify(kHs256Token, verifier, {.audiences = {"my-service"}, .type = "Application/jWt"}));
}

TEST_F(CryptoJwt, MatchesMediaTypeNameIgnoringParameters) {
    const auto token = SignRawJson(R"({"exp":2000})", R"({"alg":"HS256","typ":"Application/AT+JWT; x=/"})");
    EXPECT_NO_THROW(jwt::Verify(token, verifier, {.type = "at+jwt; profile=other"}));
    UEXPECT_THROW_MSG(jwt::Verify(token, verifier, {.type = "at+jwt; x=/"}), VerificationError, "JWT typ");
    UEXPECT_THROW_MSG(jwt::Verify(token, verifier, {.type = "text/at+jwt; x=/"}), VerificationError, "JWT typ");
}

TEST_F(CryptoJwt, PreservesOriginalSigningInput) {
    EXPECT_NO_THROW(
        jwt::Verify(SignRawJson("{\n  \"exp\":2e3, \"sub\":\"\\u0061\" }", "{ \"alg\" : \"HS256\" }"), verifier)
    );
}

}  // namespace

USERVER_NAMESPACE_END
