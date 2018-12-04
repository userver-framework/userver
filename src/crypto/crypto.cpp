#include <crypto/crypto.hpp>

#include <cryptopp/hex.h>
#include <cryptopp/misc.h>
#include <cryptopp/sha.h>

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

namespace crypto {

std::string Sha256(const std::string& data) {
  return MakeHash<CryptoPP::SHA256>(data);
}

std::string Sha512(const std::string& data) {
  return MakeHash<CryptoPP::SHA512>(data);
}

bool AreStringsEqualConstTime(const std::string& str1,
                              const std::string& str2) {
  if (str1.size() != str2.size()) return false;
  return CryptoPP::VerifyBufsEqual(reinterpret_cast<const byte*>(str1.data()),
                                   reinterpret_cast<const byte*>(str2.data()),
                                   str1.size());
}

}  // namespace crypto
