#include <utest/utest.hpp>

#include <crypto/base64.hpp>

TEST(Crypto, Base64) {
  EXPECT_EQ("", crypto::base64::Base64Encode(""));
  EXPECT_EQ("", crypto::base64::Base64Decode(""));
  EXPECT_EQ("s2323234", crypto::base64::Base64Encode(
                            crypto::base64::Base64Decode("s232$3234^&^@")));
  EXPECT_EQ("dGVzdA==", crypto::base64::Base64Encode("test"));
  EXPECT_EQ("test", crypto::base64::Base64Decode("dGVzdA=="));
  EXPECT_EQ("test", crypto::base64::Base64Decode("dGVzdA"));
  EXPECT_EQ("dGVzdA", crypto::base64::Base64Encode(
                          "test", crypto::base64::Pad::kWithout));
  EXPECT_EQ("U/8=", crypto::base64::Base64Encode("S\xff"));
  EXPECT_EQ("U_8=", crypto::base64::Base64UrlEncode("S\xff"));
  EXPECT_EQ("U_8", crypto::base64::Base64UrlEncode(
                       "S\xff", crypto::base64::Pad::kWithout));
  EXPECT_EQ("S\xFF", crypto::base64::Base64UrlDecode("U_8"));
  EXPECT_EQ("S\xFF", crypto::base64::Base64UrlDecode("U_8="));
}
