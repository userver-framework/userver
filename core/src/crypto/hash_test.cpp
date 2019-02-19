#include <utest/utest.hpp>

#include <crypto/hash.hpp>

TEST(Crypto, Sha256) {
  EXPECT_EQ("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
            crypto::hash::Sha256({}));
  EXPECT_EQ("9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08",
            crypto::hash::Sha256("test"));
  EXPECT_EQ("f2ca1bb6c7e907d06dafe4687e579fce76b37e4e93b7605022da52e6ccc26fd2",
            crypto::hash::Sha256("test\n"));
}

TEST(Crypto, Sha512) {
  EXPECT_EQ(
      "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c"
      "5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e",
      crypto::hash::Sha512({}));
  EXPECT_EQ(
      "ee26b0dd4af7e749aa1a8ee3c10ae9923f618980772e473f8819a5d4940e0db27ac185f8"
      "a0e1d5f84f88bc887fd67b143732c304cc5fa9ad8e6f57f50028a8ff",
      crypto::hash::Sha512("test"));
  EXPECT_EQ(
      "0e3e75234abc68f4378a86b3f4b32a198ba301845b0cd6e50106e874345700cc6663a86c"
      "1ea125dc5e92be17c98f9a0f85ca9d5f595db2012f7cc3571945c123",
      crypto::hash::Sha512("test\n"));
}

TEST(Crypto, Md5) {
  EXPECT_EQ("d41d8cd98f00b204e9800998ecf8427e", crypto::hash::weak::Md5({}));
  EXPECT_EQ("098f6bcd4621d373cade4e832627b4f6",
            crypto::hash::weak::Md5("test"));
  EXPECT_EQ("d8e8fca2dc0f896fd7cb4cb0031ba249",
            crypto::hash::weak::Md5("test\n"));
}
