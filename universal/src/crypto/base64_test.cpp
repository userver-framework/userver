#include <gtest/gtest.h>

#include <ostream>
#include <string>

#include <fmt/format.h>

#include <userver/crypto/base64.hpp>
#include <userver/utils/encoding/hex.hpp>

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
        Base64UniversalDecodeSuccessTestParam{"", ""},
        Base64UniversalDecodeSuccessTestParam{"SGVsbG8/", "Hello?"},
        Base64UniversalDecodeSuccessTestParam{"SGVsbG8_", "Hello?"},
        Base64UniversalDecodeSuccessTestParam{"+/s=", "\xfb\xfb"},
        Base64UniversalDecodeSuccessTestParam{"-_s=", "\xfb\xfb"},
        Base64UniversalDecodeSuccessTestParam{"+/s", "\xfb\xfb"},
        Base64UniversalDecodeSuccessTestParam{"-_s", "\xfb\xfb"}
    )
);

INSTANTIATE_TEST_SUITE_P(
    Crypto,
    Base64UniversalDecodeFailureTest,
    ::testing::Values(
        Base64UniversalDecodeFailureTestParam{"TW=E"},
        Base64UniversalDecodeFailureTestParam{"TQ==="},
        Base64UniversalDecodeFailureTestParam{"TWE#"}
    )
);

TEST(Crypto, Base64) {
    EXPECT_EQ("", crypto::base64::Base64Encode(""));
    EXPECT_EQ("", crypto::base64::Base64Decode(""));
    EXPECT_EQ("s2323234", crypto::base64::Base64Encode(crypto::base64::Base64Decode("s232$3234^&^@")));
    EXPECT_EQ("dGVzdA==", crypto::base64::Base64Encode("test"));
    EXPECT_EQ("test", crypto::base64::Base64Decode("dGVzdA=="));
    EXPECT_EQ("test", crypto::base64::Base64Decode("dGVzdA"));
    EXPECT_EQ("dGVzdA", crypto::base64::Base64Encode("test", crypto::base64::Pad::kWithout));
    EXPECT_EQ("U/8=", crypto::base64::Base64Encode("S\xff"));
}

#ifndef USERVER_NO_CRYPTOPP_BASE64_URL
TEST(Crypto, Base64Url) {
    EXPECT_EQ("U_8=", crypto::base64::Base64UrlEncode("S\xff"));
    EXPECT_EQ("U_8", crypto::base64::Base64UrlEncode("S\xff", crypto::base64::Pad::kWithout));
    EXPECT_EQ("S\xFF", crypto::base64::Base64UrlDecode("U_8"));
    EXPECT_EQ("S\xFF", crypto::base64::Base64UrlDecode("U_8="));
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
