#include <gtest/gtest.h>

#include <optional>
#include <ostream>
#include <random>
#include <string>

#include <fmt/format.h>

#include <userver/crypto/base64.hpp>
#include <userver/crypto/exception.hpp>
#include <userver/utils/encoding/hex.hpp>

#include <crypto/impl/base64_cryptopp.hpp>
#include <crypto/impl/base64_simd.hpp>

USERVER_NAMESPACE_BEGIN

struct Base64UniversalDecodeSuccessTestParam {
    std::string input;
    std::string expected_result;
};

struct Base64UniversalDecodeFailureTestParam {
    std::string input;
};

class Base64UniversalDecodeSuccessTest : public ::testing::TestWithParam<Base64UniversalDecodeSuccessTestParam> {};
class Base64UniversalDecodeFailureTest : public ::testing::TestWithParam<Base64UniversalDecodeFailureTestParam> {};

void PrintTo(const Base64UniversalDecodeSuccessTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

void PrintTo(const Base64UniversalDecodeFailureTestParam& param, std::ostream* os) {
    *os << fmt::format("{{ input = '{}' }}", param.input);
}

INSTANTIATE_TEST_SUITE_P(
    Crypto,
    Base64UniversalDecodeSuccessTest,
    ::testing::Values(
        Base64UniversalDecodeSuccessTestParam{.input = "", .expected_result = ""},
        Base64UniversalDecodeSuccessTestParam{.input = "SGVsbG8/", .expected_result = "Hello?"},
        Base64UniversalDecodeSuccessTestParam{.input = "SGVsbG8_", .expected_result = "Hello?"},
        Base64UniversalDecodeSuccessTestParam{.input = "+/s=", .expected_result = "\xfb\xfb"},
        Base64UniversalDecodeSuccessTestParam{.input = "-_s=", .expected_result = "\xfb\xfb"},
        Base64UniversalDecodeSuccessTestParam{.input = "+/s", .expected_result = "\xfb\xfb"},
        Base64UniversalDecodeSuccessTestParam{.input = "-_s", .expected_result = "\xfb\xfb"}
    )
);

INSTANTIATE_TEST_SUITE_P(
    Crypto,
    Base64UniversalDecodeFailureTest,
    ::testing::Values(
        Base64UniversalDecodeFailureTestParam{.input = "TW=E"},
        Base64UniversalDecodeFailureTestParam{.input = "TQ==="},
        Base64UniversalDecodeFailureTestParam{.input = "TWE#"}
    )
);

TEST(Crypto, Base64) {
    EXPECT_EQ("", crypto::base64::Base64Encode(""));
    EXPECT_EQ("", crypto::base64::Base64Decode(""));
    EXPECT_THROW(crypto::base64::Base64Decode("s232$3234^&^@"), crypto::CryptoException);
    EXPECT_EQ("dGVzdA==", crypto::base64::Base64Encode("test"));
    EXPECT_EQ("test", crypto::base64::Base64Decode("dGVzdA=="));
    EXPECT_EQ("test", crypto::base64::Base64Decode("dGVzdA"));
    EXPECT_EQ("dGVzdA", crypto::base64::Base64Encode("test", crypto::base64::Pad::kWithout));
    EXPECT_EQ("U/8=", crypto::base64::Base64Encode("S\xff"));
}

namespace {

std::string MakeBinaryData(std::size_t size) {
    std::string data(size, '\0');
    for (std::size_t i = 0; i < size; ++i) {
        data[i] = static_cast<char>(i * 131 + size * 7);
    }
    return data;
}

// The output of std::mt19937 is fixed by the standard, unlike distributions, so failures are reproducible
std::string MakeRandomData(std::mt19937& gen, std::size_t size) {
    std::string data(size, '\0');
    for (auto& c : data) {
        c = static_cast<char>(gen());
    }
    return data;
}

void ExpectMatchesCryptoPP(const std::string& data) {
    for (const auto pad : {crypto::base64::Pad::kWith, crypto::base64::Pad::kWithout}) {
        const auto encoded = crypto::base64::Base64Encode(data, pad);
        EXPECT_EQ(encoded, crypto::base64::impl::CryptoPPBase64Encode(data, pad)) << data.size();
        EXPECT_EQ(crypto::base64::Base64Decode(encoded), data) << data.size();
#ifndef USERVER_NO_CRYPTOPP_BASE64_URL
        const auto url_encoded = crypto::base64::Base64UrlEncode(data, pad);
        EXPECT_EQ(url_encoded, crypto::base64::impl::CryptoPPBase64UrlEncode(data, pad)) << data.size();
        EXPECT_EQ(crypto::base64::Base64UrlDecode(url_encoded), data) << data.size();
#endif
    }
}

#ifndef USERVER_NO_SIMD_BASE64
// Mostly valid characters of both alphabets, padding and characters that some decoders treat specially
constexpr char kBase64LikeChars[] = "AQgwBRhx09+/+/-_-_====,, \n\r\t.\0\xff";

char MakeBase64LikeChar(std::mt19937& gen) { return kBase64LikeChars[gen() % (sizeof(kBase64LikeChars) - 1)]; }

std::string MakeBase64LikeData(std::mt19937& gen, std::size_t size) {
    std::string data(size, '\0');
    for (auto& c : data) {
        c = MakeBase64LikeChar(gen);
    }
    return data;
}

// The SIMD decoder accepts the same input as IsCanonicalBase64 and decodes it the same way as crypto++ does.
// It is called directly: Base64Decode throws on rejected input, and exceptions are too slow for this many inputs.
void ExpectSimdDecodesAsCryptoPPOrRejects(std::string_view input) {
    using crypto::base64::impl::Alphabet;
    const auto expected =
        crypto::base64::impl::IsCanonicalBase64(input)
            ? std::optional{crypto::base64::impl::CryptoPPBase64Decode(input)}
            : std::nullopt;
    EXPECT_EQ(crypto::base64::impl::SimdBase64Decode(input, Alphabet::kStandard), expected)
        << utils::encoding::ToHex(input);
#ifndef USERVER_NO_CRYPTOPP_BASE64_URL
    const auto expected_url =
        crypto::base64::impl::IsCanonicalBase64Url(input)
            ? std::optional{crypto::base64::impl::CryptoPPBase64UrlDecode(input)}
            : std::nullopt;
    EXPECT_EQ(crypto::base64::impl::SimdBase64Decode(input, Alphabet::kUrl), expected_url)
        << utils::encoding::ToHex(input);
#endif
}
#endif

}  // namespace

TEST(Crypto, Base64MatchesCryptoPP) {
    // Covers the scalar tails and the SIMD loops of every codec
    for (std::size_t size = 0; size < 300 && !HasFailure(); ++size) {
        ExpectMatchesCryptoPP(MakeBinaryData(size));
    }
    std::mt19937 gen{42};
    for (std::size_t size = 0; size < 1024 && !HasFailure(); ++size) {
        ExpectMatchesCryptoPP(MakeRandomData(gen, size));
    }
}

#ifndef USERVER_NO_SIMD_BASE64
TEST(Crypto, Base64SimdDecodeArbitraryInput) {
    std::mt19937 gen{42};
    for (int i = 0; i < 20'000 && !HasFailure(); ++i) {
        ExpectSimdDecodesAsCryptoPPOrRejects(MakeBase64LikeData(gen, gen() % 40));

        // A long valid encoding with one character replaced, so that the SIMD loops meet the bad character too
        const auto pad = gen() % 2 ? crypto::base64::Pad::kWith : crypto::base64::Pad::kWithout;
        auto input = crypto::base64::impl::CryptoPPBase64Encode(MakeRandomData(gen, gen() % 200), pad);
        if (!input.empty()) {
            input[gen() % input.size()] = MakeBase64LikeChar(gen);
        }
        ExpectSimdDecodesAsCryptoPPOrRejects(input);
    }
}
#endif

TEST(Crypto, Base64DecodeUnpaddedAndNonZeroTrailingBits) {
    EXPECT_EQ(crypto::base64::Base64Decode("dGVzdA"), "test");
    EXPECT_EQ(crypto::base64::Base64Decode("QQ="), "A");
    EXPECT_EQ(crypto::base64::Base64Decode("QR"), "A");
    EXPECT_EQ(crypto::base64::Base64Decode("QR=="), "A");
}

TEST(Crypto, Base64DecodeSkipsLineBreaks) {
    EXPECT_EQ(crypto::base64::Base64Decode("dGVz\ndA==\n"), "test");
    EXPECT_EQ(crypto::base64::Base64Decode("dGVz\r\ndA==\r\n"), "test");
    EXPECT_EQ(crypto::base64::Base64Decode("\ndGVzdA=\n="), "test");
    EXPECT_EQ(crypto::base64::Base64Decode("\r\n"), "");

    // MIME-style lines of 76 characters
    const auto data = MakeBinaryData(200);
    const auto encoded = crypto::base64::Base64Encode(data);
    std::string wrapped;
    for (std::size_t i = 0; i < encoded.size(); i += 76) {
        wrapped += encoded.substr(i, 76);
        wrapped += "\r\n";
    }
    EXPECT_EQ(crypto::base64::Base64Decode(wrapped), data);
}

TEST(Crypto, Base64DecodeRejectsNonCanonical) {
    for (const std::string_view input : {
             " dGVzdA==",
             "dGVz dA==",
             "dGVz\tdA==",
             "Q\n",
             "QQ=\n=\n=",
             "QQ==QUJD",
             "QQ,,QUJD",
             "QQ,,",
             "QQ=A",
             "Q",
             "Q===",
             "QUJDR=",
             "=",
             "====",
             "-_8=",
             "U/8=U/8=",
             "s232$3234^&^@",
             "\xff\xfe\xfd\xfc",
         })
    {
        EXPECT_FALSE(crypto::base64::impl::IsCanonicalBase64(input)) << input;
        EXPECT_THROW(crypto::base64::Base64Decode(input), crypto::CryptoException) << input;
    }
}

#ifndef USERVER_NO_CRYPTOPP_BASE64_URL
TEST(Crypto, Base64Url) {
    EXPECT_EQ("U_8=", crypto::base64::Base64UrlEncode("S\xff"));
    EXPECT_EQ("U_8", crypto::base64::Base64UrlEncode("S\xff", crypto::base64::Pad::kWithout));
    EXPECT_EQ("S\xFF", crypto::base64::Base64UrlDecode("U_8"));
    EXPECT_EQ("S\xFF", crypto::base64::Base64UrlDecode("U_8="));
}

TEST(Crypto, Base64UrlDecodeUnpaddedAndNonZeroTrailingBits) {
    EXPECT_EQ(crypto::base64::Base64UrlDecode("_-_-"), "\xff\xef\xfe");
    EXPECT_EQ(crypto::base64::Base64UrlDecode("QQ="), "A");
    EXPECT_EQ(crypto::base64::Base64UrlDecode("QR"), "A");
}

TEST(Crypto, Base64UrlDecodeSkipsLineBreaks) { EXPECT_EQ(crypto::base64::Base64UrlDecode("U_\r\n8\n"), "S\xFF"); }

TEST(Crypto, Base64UrlDecodeRejectsNonCanonical) {
    for (const std::string_view input : {
             "U_8==",
             "U/8=",
             "U+8",
             "U_8 ",
             "Q",
             "QQ==QUJD",
             "s232$3234^&^@",
         })
    {
        EXPECT_FALSE(crypto::base64::impl::IsCanonicalBase64Url(input)) << input;
        EXPECT_THROW(crypto::base64::Base64UrlDecode(input), crypto::CryptoException) << input;
    }
}
#endif

TEST_P(Base64UniversalDecodeSuccessTest, Test) {
    const auto& param = GetParam();
    std::string data = param.input;

    ASSERT_TRUE(crypto::base64::Base64UniversalDecodeInPlace(data));
    EXPECT_EQ(data, param.expected_result);
}

TEST_P(Base64UniversalDecodeFailureTest, Test) {
    const auto& param = GetParam();
    std::string data = param.input;

    EXPECT_FALSE(crypto::base64::Base64UniversalDecodeInPlace(data));
}

USERVER_NAMESPACE_END
