#include <userver/crypto/jwt.hpp>

#include <algorithm>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

#include <fmt/format.h>
#include <gtest/gtest.h>

#include <userver/crypto/base64.hpp>
#include <userver/crypto/signers.hpp>
#include <userver/crypto/verifiers.hpp>
#include <userver/utils/mock_now.hpp>

#include "jwt_test_data.hpp"

USERVER_NAMESPACE_BEGIN

namespace {

using crypto::VerificationError;
namespace jwt = crypto::jwt;
using namespace std::chrono_literals;

std::string Encode(std::string_view value) {
    auto encoded = crypto::base64::Base64Encode(value, crypto::base64::Pad::kWithout);
    std::ranges::replace(encoded, '+', '-');
    std::ranges::replace(encoded, '/', '_');
    return encoded;
}

std::string MakeToken(std::string_view payload, std::string_view header = R"({"alg":"HS256"})") {
    const auto input = fmt::format("{}.{}", Encode(header), Encode(payload));
    const auto signature = crypto::SignerHs256{std::string{kHmacKey}}.Sign({input});
    return fmt::format("{}.{}", input, Encode(signature));
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
    const auto verified = jwt::Verify(test.token, *signature_verifier, {.issuer = "issuer", .audience = "my-service"});
    EXPECT_EQ(verified.header["alg"].As<std::string>(), test.algorithm);
    EXPECT_EQ(verified.payload["sub"].As<std::string>(), "user-42");

    std::string tampered{test.token};
    const auto signature_start = tampered.rfind('.') + 1;
    tampered[signature_start] = tampered[signature_start] == 'A' ? 'B' : 'A';
    EXPECT_THROW(jwt::Verify(tampered, *signature_verifier), VerificationError);
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
    const auto
        verified = crypto::jwt::Verify(token, verifier, {.issuer = "issuer", .audience = "my-service", .type = "JWT"});
    const auto subject = verified.payload["sub"].As<std::string>();
    const auto scope = verified.payload["scope"].As<std::string>();
    /// [jwt verify]
    EXPECT_EQ(subject, "user-42");
    EXPECT_EQ(scope, "read");
}

TEST_F(CryptoJwt, ParseOwnsUnverifiedData) {
    auto token = MakeToken(R"({"exp":1,"sub":"untrusted"})");
    const auto parsed = jwt::Parse(token);
    token.clear();
    EXPECT_EQ(parsed.payload["sub"].As<std::string>(), "untrusted");
    EXPECT_EQ(parsed.header["alg"].As<std::string>(), "HS256");
}

TEST_F(CryptoJwt, WrongKeyAndTamperedContent) {
    EXPECT_THROW(jwt::Verify(kHs256Token, crypto::VerifierHs256{std::string(64, 'x')}), VerificationError);
    const std::string token{kHs256Token};
    const auto first = token.find('.');
    const auto second = token.find('.', first + 1);
    EXPECT_THROW(
        jwt::Verify(
            fmt::format("{}.{}", Encode(R"({"alg":"HS256","kid":"changed"})"), token.substr(first + 1)),
            verifier
        ),
        VerificationError
    );
    EXPECT_THROW(
        jwt::Verify(
            fmt::format(
                "{}.{}.{}",
                token.substr(0, first),
                Encode(R"({"exp":2000,"sub":"admin"})"),
                token.substr(second + 1)
            ),
            verifier
        ),
        VerificationError
    );
}

TEST_F(CryptoJwt, AlgorithmCannotBeSelectedByToken) {
    for (const auto algorithm : {"none", "HS1", "RS256", "HS384", "hs256", "unknown"}) {
        SCOPED_TRACE(algorithm);
        const auto token = MakeToken(R"({"exp":2000})", fmt::format(R"({{"alg":"{}"}})", algorithm));
        EXPECT_THROW(jwt::Verify(token, verifier), VerificationError);
    }
    EXPECT_THROW(
        jwt::Verify(MakeToken(R"({"exp":2000})", R"({"alg":"none"})"), crypto::VerifierNone{}),
        VerificationError
    );

    const auto confused = fmt::format("{}.{}", Encode(R"({"alg":"HS256"})"), Encode(R"({"exp":2000})"));
    const auto forged =
        fmt::format("{}.{}", confused, Encode(crypto::SignerHs256{std::string{kRsPublicKey}}.Sign({confused})));
    EXPECT_THROW(jwt::Verify(forged, crypto::VerifierRs256{kRsPublicKey}), VerificationError);
}

TEST_F(CryptoJwt, RejectsMalformedCompactEncoding) {
    for (const auto token : {"", "abc", "a.b", "a.b.c.d", "a.b.c.d.e", ".e30.AA", "e30..AA"}) {
        SCOPED_TRACE(token);
        EXPECT_THROW(jwt::Parse(token), VerificationError);
    }
    const auto prefix = fmt::format("{}.{}", Encode(R"({"alg":"HS256"})"), Encode(R"({"exp":2000})"));
    for (const auto signature : {"", "A", "AA=", "A+", "A/", "A A", "AB"}) {
        SCOPED_TRACE(signature);
        EXPECT_THROW(jwt::Parse(fmt::format("{}.{}", prefix, signature)), VerificationError);
    }
    const auto valid = MakeToken(R"({"exp":2000})");
    for (const auto position : {std::size_t{0}, valid.find('.') + 1, valid.rfind('.') + 1}) {
        auto invalid = valid;
        invalid.insert(position, "=");
        EXPECT_THROW(jwt::Verify(invalid, verifier), VerificationError);
    }
}

TEST_F(CryptoJwt, RejectsInvalidJson) {
    for (const auto json :
         {"",
          "null",
          "[]",
          "42",
          "{",
          "{}{}",
          R"({"alg":"HS256","alg":"none"})",
          R"({"sub":"user","sub":"admin"})",
          R"({"a":{"b":1,"b":2}})",
          R"({"a":"\ud800"})"})
    {
        SCOPED_TRACE(json);
        EXPECT_THROW(jwt::Parse(MakeToken(json)), VerificationError);
        EXPECT_THROW(jwt::Parse(MakeToken("{}", json)), VerificationError);
    }
    EXPECT_THROW(jwt::Parse(MakeToken(std::string{"{\"sub\":\"\xff\"}"})), VerificationError);
}

TEST_F(CryptoJwt, RejectsInvalidAlgorithmAndUnsupportedHeaders) {
    for (const auto header :
         {R"({})",
          R"({"alg":null})",
          R"({"alg":1})",
          R"({"alg":""})",
          R"({"alg":"HS256","crit":["custom"],"custom":true})",
          R"({"alg":"HS256","crit":[]})",
          R"({"alg":"HS256","crit":null})",
          R"({"alg":"HS256","b64":false})",
          R"({"alg":"HS256","b64":true})",
          R"({"alg":"HS256","cty":"jwt"})",
          R"({"alg":"HS256","cty":"application/JWT"})"})
    {
        SCOPED_TRACE(header);
        EXPECT_THROW(jwt::Verify(MakeToken(R"({"exp":2000})", header), verifier), VerificationError);
    }
}

TEST_F(CryptoJwt, ExpirationAndClockSkewBoundaries) {
    const auto expired = MakeToken(R"({"exp":1000})");
    EXPECT_THROW(jwt::Verify(expired, verifier), VerificationError);
    EXPECT_NO_THROW(jwt::Verify(expired, verifier, {.leeway = 1s}));
    EXPECT_THROW(jwt::Verify(MakeToken(R"({"exp":999})"), verifier, {.leeway = 1s}), VerificationError);
    EXPECT_NO_THROW(jwt::Verify(MakeToken(R"({"exp":1000.5})"), verifier));
    utils::datetime::MockNowSet(std::chrono::system_clock::time_point{1000s + 500ms});
    EXPECT_THROW(jwt::Verify(MakeToken(R"({"exp":1000.5})"), verifier), VerificationError);
}

TEST_F(CryptoJwt, NotBeforeAndIssuedAtBoundaries) {
    for (const auto claim : {"nbf", "iat"}) {
        SCOPED_TRACE(claim);
        EXPECT_NO_THROW(jwt::Verify(MakeToken(fmt::format(R"({{"exp":2000,"{}":1000}})", claim)), verifier));
        const auto future = MakeToken(fmt::format(R"({{"exp":2000,"{}":1001}})", claim));
        EXPECT_THROW(jwt::Verify(future, verifier), VerificationError);
        EXPECT_NO_THROW(jwt::Verify(future, verifier, {.leeway = 1s}));
    }
}

TEST_F(CryptoJwt, ExpirationPolicy) {
    EXPECT_THROW(jwt::Verify(MakeToken("{}"), verifier), VerificationError);
    const jwt::VerificationOptions optional{.expiration = jwt::ExpirationPolicy::kOptional};
    EXPECT_NO_THROW(jwt::Verify(MakeToken("{}"), verifier, optional));
    EXPECT_THROW(jwt::Verify(MakeToken(R"({"exp":999})"), verifier, optional), VerificationError);
    EXPECT_THROW(jwt::Verify(kHs256Token, verifier, {.leeway = -1s}), std::invalid_argument);
}

TEST_F(CryptoJwt, NumericDateTypesAndRange) {
    for (const auto claim : {"exp", "nbf", "iat"}) {
        for (const auto value : {"null", "true", "\"1000\"", "[]", "{}", "1e400"}) {
            SCOPED_TRACE(fmt::format("{}={}", claim, value));
            EXPECT_THROW(
                jwt::Verify(
                    MakeToken(fmt::format(R"({{"{}":{}}})", claim, value)),
                    verifier,
                    {.expiration = jwt::ExpirationPolicy::kOptional}
                ),
                VerificationError
            );
        }
    }
    EXPECT_NO_THROW(
        jwt::Verify(MakeToken(R"({"exp":18446744073709551615,"nbf":-9223372036854775808,"iat":-1e100})"), verifier)
    );
    EXPECT_THROW(jwt::Verify(MakeToken(R"({"exp":-9223372036854775808})"), verifier), VerificationError);
}

TEST_F(CryptoJwt, ExpectedClaims) {
    const jwt::VerificationOptions
        expected{.issuer = "issuer", .audience = "my-service", .subject = "user-42", .type = "JWT"};
    EXPECT_NO_THROW(jwt::Verify(kHs256Token, verifier, expected));
    EXPECT_THROW(jwt::Verify(kHs256Token, verifier, {.issuer = "another-issuer"}), VerificationError);
    EXPECT_THROW(jwt::Verify(kHs256Token, verifier, {.audience = "another-service"}), VerificationError);
    EXPECT_THROW(jwt::Verify(kHs256Token, verifier, {.subject = "another-user"}), VerificationError);
    EXPECT_THROW(jwt::Verify(kHs256Token, verifier, {.type = "at+jwt"}), VerificationError);
    for (const auto& options :
         {jwt::VerificationOptions{.issuer = "issuer"},
          jwt::VerificationOptions{.audience = "service"},
          jwt::VerificationOptions{.subject = "user"},
          jwt::VerificationOptions{.type = "JWT"}})
    {
        EXPECT_THROW(jwt::Verify(MakeToken(R"({"exp":2000})"), verifier, options), VerificationError);
    }
    EXPECT_NO_THROW(jwt::Verify(MakeToken(R"({"exp":2000,"aud":"my-service"})"), verifier, {.audience = "my-service"}));
    EXPECT_THROW(
        jwt::Verify(MakeToken(R"({"exp":2000,"aud":[]})"), verifier, {.audience = "my-service"}),
        VerificationError
    );
}

TEST_F(CryptoJwt, RegisteredClaimTypes) {
    for (const auto claim : {"iss", "sub", "jti", "aud"}) {
        for (const auto value : {"null", "123", "true", "{}"}) {
            SCOPED_TRACE(fmt::format("{}={}", claim, value));
            EXPECT_THROW(
                jwt::Verify(MakeToken(fmt::format(R"({{"exp":2000,"{}":{}}})", claim, value)), verifier),
                VerificationError
            );
        }
    }
    EXPECT_THROW(
        jwt::Verify(MakeToken(R"({"exp":2000,"aud":["my-service",null]})"), verifier, {.audience = "my-service"}),
        VerificationError
    );
    for (const auto header : {"typ", "kid", "cty"}) {
        EXPECT_THROW(
            jwt::Verify(MakeToken(R"({"exp":2000})", fmt::format(R"({{"alg":"HS256","{}":null}})", header)), verifier),
            VerificationError
        );
    }
}

TEST_F(CryptoJwt, PreservesOriginalSigningInput) {
    EXPECT_NO_THROW(
        jwt::Verify(MakeToken("{\n  \"exp\":2e3, \"sub\":\"\\u0061\" }", "{ \"alg\" : \"HS256\" }"), verifier)
    );
}

TEST_F(CryptoJwt, RejectsNonstandardEcdsaSignatureLength) {
    const auto separator = kEs256Token.rfind('.');
    std::string encoded{kEs256Token.substr(separator + 1)};
    std::ranges::replace(encoded, '-', '+');
    std::ranges::replace(encoded, '_', '/');
    auto signature = crypto::base64::Base64Decode(encoded);
    signature.insert(signature.begin(), '\0');
    signature.insert(signature.begin() + 33, '\0');
    EXPECT_THROW(
        jwt::Verify(
            fmt::format("{}.{}", kEs256Token.substr(0, separator), Encode(signature)),
            crypto::VerifierEs256{kEs256PublicKey}
        ),
        VerificationError
    );
}

}  // namespace

USERVER_NAMESPACE_END
