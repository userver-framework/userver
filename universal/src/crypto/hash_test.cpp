#include <gtest/gtest.h>

#include <userver/crypto/hash.hpp>

USERVER_NAMESPACE_BEGIN

TEST(Crypto, Sha1) {
  EXPECT_EQ("da39a3ee5e6b4b0d3255bfef95601890afd80709", crypto::hash::Sha1({}));
  EXPECT_EQ("a94a8fe5ccb19ba61c4c0873d391e987982fbbd3",
            crypto::hash::Sha1("test"));
  EXPECT_EQ("4e1243bd22c66e76c2ba9eddc1f91394e57f9f83",
            crypto::hash::Sha1("test\n"));
}

TEST(Crypto, Sha224) {
  EXPECT_EQ("d14a028c2a3a2bc9476102bb288234c415a2b01f828ea62ac5b3e42f",
            crypto::hash::Sha224({}));
  EXPECT_EQ("90a3ed9e32b2aaf4c61c410eb925426119e1a9dc53d4286ade99a809",
            crypto::hash::Sha224("test"));
  EXPECT_EQ("52f1bf093f4b7588726035c176c0cdb4376cfea53819f1395ac9e6ec",
            crypto::hash::Sha224("test\n"));
}

TEST(Crypto, Sha256) {
  EXPECT_EQ("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
            crypto::hash::Sha256({}));
  EXPECT_EQ("9f86d081884c7d659a2feaa0c55ad015a3bf4f1b2b0b822cd15d6c15b0f00a08",
            crypto::hash::Sha256("test"));
  EXPECT_EQ("f2ca1bb6c7e907d06dafe4687e579fce76b37e4e93b7605022da52e6ccc26fd2",
            crypto::hash::Sha256("test\n"));
}

TEST(Crypto, Sha384) {
  EXPECT_EQ(
      "38b060a751ac96384cd9327eb1b1e36a21fdb71114be07434c0cc7bf63f6e1da274edebf"
      "e76f65fbd51ad2f14898b95b",
      crypto::hash::Sha384({}));
  EXPECT_EQ(
      "768412320f7b0aa5812fce428dc4706b3cae50e02a64caa16a782249bfe8efc4b7ef1ccb"
      "126255d196047dfedf17a0a9",
      crypto::hash::Sha384("test"));
  EXPECT_EQ(
      "109bb6b5b6d5547c1ce03c7a8bd7d8f80c1cb0957f50c4f7fda04692079917e4f9cad52b"
      "878f3d8234e1a170b154b72d",
      crypto::hash::Sha384("test\n"));
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

TEST(Crypto, Hmac) {
  EXPECT_EQ(
      "9ba1f63365a6caf66e46348f43cdef956015bea997adeb06e69007ee3ff517df10fc5eb8"
      "60da3d43b82c2a040c931119d2dfc6d08e253742293a868cc2d82015",
      crypto::hash::HmacSha512("test", "test",
                               crypto::hash::OutputEncoding::kHex));
  EXPECT_EQ(
      "m6H2M2WmyvZuRjSPQ83vlWAVvqmXresG5pAH7j/1F98Q/F64YNo9Q7gsKgQMkxEZ0t/"
      "G0I4lN0IpOoaMwtggFQ==",
      crypto::hash::HmacSha512("test", "test",
                               crypto::hash::OutputEncoding::kBase64));

  EXPECT_EQ("88cd2108b5347d973cf39cdf9053d7dd42704876d8c9a9bd8e2d168259d3ddf7",
            crypto::hash::HmacSha256("test", "test",
                                     crypto::hash::OutputEncoding::kHex));

  EXPECT_EQ("0c94515c15e5095b8a87a50ba0df3bf38ed05fe6",
            crypto::hash::HmacSha1("test", "test",
                                   crypto::hash::OutputEncoding::kHex));
  EXPECT_EQ(
      "b818f4664d0826b102b72cf2a687f558368f2152b15b83a7f389e48c335fc455282c61e9"
      "7335dae370bac31a8196772d",
      crypto::hash::HmacSha384("secret", "",
                               crypto::hash::OutputEncoding::kHex));
}

#ifndef USERVER_NO_CRYPTOPP_BLAKE2
TEST(Crypto, Blake2b128) {
  EXPECT_EQ("e9a804b2e527fd3601d2ffc0bb023cd6",
            crypto::hash::Blake2b128("hello world",
                                     crypto::hash::OutputEncoding::kHex));
  EXPECT_EQ("cae66941d9efbd404e4d88758ea67670",
            crypto::hash::Blake2b128("", crypto::hash::OutputEncoding::kHex));
}
#endif

USERVER_NAMESPACE_END
