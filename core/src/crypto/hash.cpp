#include <crypto/hash.hpp>

#include <cryptopp/hex.h>
#include <cryptopp/sha.h>

#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include <cryptopp/md5.h>
#undef CRYPTOPP_ENABLE_NAMESPACE_WEAK

#ifdef CRYPTOPP_NO_GLOBAL_BYTE
using CryptoPP::byte;
#endif

namespace {

template <typename Hash>
std::string MakeHash(const std::string& data) {
  byte digest[Hash::DIGESTSIZE];

  Hash hash;
  hash.CalculateDigest(digest, reinterpret_cast<const byte*>(data.data()),
                       data.size());

  CryptoPP::HexEncoder encoder(nullptr, false);
  std::string output;

  encoder.Attach(new CryptoPP::StringSink(output));
  encoder.Put(digest, sizeof(digest));
  encoder.MessageEnd();

  return output;
}

}  // namespace

namespace crypto::hash {

std::string Sha256(const std::string& data) {
  return MakeHash<CryptoPP::SHA256>(data);
}

std::string Sha512(const std::string& data) {
  return MakeHash<CryptoPP::SHA512>(data);
}

namespace weak {

std::string Md5(const std::string& data) {
  return MakeHash<CryptoPP::Weak::MD5>(data);
}

}  // namespace weak
}  // namespace crypto::hash
