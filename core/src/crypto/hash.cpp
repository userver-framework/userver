#include <crypto/hash.hpp>

#include <cryptopp/base64.h>
#include <cryptopp/blake2.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>
#include <cryptopp/hmac.h>
#include <cryptopp/sha.h>

#include <crypto/exception.hpp>

#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1
#include <cryptopp/md5.h>
#undef CRYPTOPP_ENABLE_NAMESPACE_WEAK

#ifdef CRYPTOPP_NO_GLOBAL_BYTE
using CryptoPP::byte;
#endif

namespace {

// Custom class for specific default initialization for Blake2b
class AlgoBlake2b128 final : public CryptoPP::BLAKE2b {
 public:
  AlgoBlake2b128() : CryptoPP::BLAKE2b(false, 16) {}
  static constexpr size_t DIGESTSIZE{16};
};

std::string EncodeArray(const byte* ptr, size_t length,
                        crypto::hash::OutputEncoding encoding) {
  std::string response;
  switch (encoding) {
    case crypto::hash::OutputEncoding::kBinary: {
      response = std::string(reinterpret_cast<const char*>(ptr), length);
      break;
    }
    case crypto::hash::OutputEncoding::kBase16: {
      CryptoPP::HexEncoder encoder(new CryptoPP::StringSink(response), false);
      encoder.PutMessageEnd(ptr, length);
      break;
    }
    case crypto::hash::OutputEncoding::kBase64: {
      CryptoPP::Base64Encoder encoder(new CryptoPP::StringSink(response),
                                      false);
      encoder.PutMessageEnd(ptr, length);
      break;
    }
  }
  return response;
}

std::string EncodeString(utils::string_view data,
                         crypto::hash::OutputEncoding encoding) {
  return EncodeArray(reinterpret_cast<const byte*>(data.data()), data.size(),
                     encoding);
}

template <typename HashAlgorithm>
std::string CalculateHmac(utils::string_view key, utils::string_view data,
                          crypto::hash::OutputEncoding encoding) {
  std::string mac;

  try {
    CryptoPP::HMAC<HashAlgorithm> hmac(
        reinterpret_cast<const byte*>(key.data()), key.size());
    CryptoPP::StringSource ss_key(
        reinterpret_cast<const byte*>(data.data()), data.size(), true,
        new CryptoPP::HashFilter(hmac, new CryptoPP::StringSink(mac)));
  } catch (const CryptoPP::Exception& exc) {
    throw crypto::CryptoException(exc.what());
  }

  return EncodeString(mac, encoding);
}

template <typename HashAlgorithm>
std::string CalculateHash(utils::string_view data,
                          crypto::hash::OutputEncoding encoding) {
  byte digest[HashAlgorithm::DIGESTSIZE];
  try {
    HashAlgorithm hash;
    hash.CalculateDigest(digest, reinterpret_cast<const byte*>(data.data()),
                         data.size());
  } catch (const CryptoPP::Exception& exc) {
    throw crypto::CryptoException(exc.what());
  }

  return EncodeArray(digest, sizeof(digest), encoding);
}

}  // namespace

namespace crypto::hash {

std::string Blake2b128(utils::string_view data, OutputEncoding encoding) {
  return CalculateHash<AlgoBlake2b128>(data, encoding);
}

std::string Sha1(utils::string_view data, OutputEncoding encoding) {
  return CalculateHash<CryptoPP::SHA1>(data, encoding);
}

std::string Sha224(utils::string_view data, OutputEncoding encoding) {
  return CalculateHash<CryptoPP::SHA224>(data, encoding);
}

std::string Sha256(utils::string_view data, OutputEncoding encoding) {
  return CalculateHash<CryptoPP::SHA256>(data, encoding);
}

std::string Sha384(utils::string_view data, OutputEncoding encoding) {
  return CalculateHash<CryptoPP::SHA384>(data, encoding);
}

std::string Sha512(utils::string_view data, OutputEncoding encoding) {
  return CalculateHash<CryptoPP::SHA512>(data, encoding);
}

std::string HmacSha512(utils::string_view key, utils::string_view message,
                       OutputEncoding encoding) {
  return CalculateHmac<CryptoPP::SHA512>(key, message, encoding);
}

std::string HmacSha384(utils::string_view key, utils::string_view message,
                       OutputEncoding encoding) {
  return CalculateHmac<CryptoPP::SHA384>(key, message, encoding);
}

std::string HmacSha256(utils::string_view key, utils::string_view message,
                       OutputEncoding encoding) {
  return CalculateHmac<CryptoPP::SHA256>(key, message, encoding);
}

std::string HmacSha1(utils::string_view key, utils::string_view message,
                     OutputEncoding encoding) {
  return CalculateHmac<CryptoPP::SHA1>(key, message, encoding);
}

namespace weak {

std::string Md5(utils::string_view data, OutputEncoding encoding) {
  return CalculateHash<CryptoPP::Weak::MD5>(data, encoding);
}

}  // namespace weak
}  // namespace crypto::hash
