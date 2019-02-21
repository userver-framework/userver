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
}
