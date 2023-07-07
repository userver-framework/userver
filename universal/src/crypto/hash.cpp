#include <userver/crypto/hash.hpp>

#include <array>

#include <cryptopp/base64.h>
#ifndef USERVER_NO_CRYPTOPP_BLAKE2
#include <cryptopp/blake2.h>
#endif
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>
#include <cryptopp/hmac.h>
#include <cryptopp/sha.h>

#include <userver/crypto/exception.hpp>

#include <cryptopp/md5.h>

#ifdef CRYPTOPP_NO_GLOBAL_BYTE
using CryptoPP::byte;
#endif

USERVER_NAMESPACE_BEGIN

namespace {

#ifndef USERVER_NO_CRYPTOPP_BLAKE2
// Custom class for specific default initialization for Blake2b
class AlgoBlake2b128 final : public CryptoPP::BLAKE2b {
 public:
  AlgoBlake2b128() : CryptoPP::BLAKE2b(false, 16) {}
  static constexpr size_t DIGESTSIZE{16};
};
#endif

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

std::string EncodeString(std::string_view data,
                         crypto::hash::OutputEncoding encoding) {
  return EncodeArray(reinterpret_cast<const byte*>(data.data()), data.size(),
                     encoding);
}

template <typename HashAlgorithm>
std::string CalculateHmac(std::string_view key, std::string_view data,
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
std::string CalculateHash(std::string_view data,
                          crypto::hash::OutputEncoding encoding) {
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init): performance
  std::array<byte, HashAlgorithm::DIGESTSIZE> digest;
  try {
    HashAlgorithm hash;
    hash.CalculateDigest(
        digest.data(), reinterpret_cast<const byte*>(data.data()), data.size());
  } catch (const CryptoPP::Exception& exc) {
    throw crypto::CryptoException(exc.what());
  }

  return EncodeArray(digest.data(), digest.size(), encoding);
}

}  // namespace

namespace crypto::hash {

#ifndef USERVER_NO_CRYPTOPP_BLAKE2
std::string Blake2b128(std::string_view data, OutputEncoding encoding) {
  return CalculateHash<AlgoBlake2b128>(data, encoding);
}
#endif

std::string Sha1(std::string_view data, OutputEncoding encoding) {
  return CalculateHash<CryptoPP::SHA1>(data, encoding);
}

std::string Sha224(std::string_view data, OutputEncoding encoding) {
  return CalculateHash<CryptoPP::SHA224>(data, encoding);
}

std::string Sha256(std::string_view data, OutputEncoding encoding) {
  return CalculateHash<CryptoPP::SHA256>(data, encoding);
}

std::string Sha384(std::string_view data, OutputEncoding encoding) {
  return CalculateHash<CryptoPP::SHA384>(data, encoding);
}

std::string Sha512(std::string_view data, OutputEncoding encoding) {
  return CalculateHash<CryptoPP::SHA512>(data, encoding);
}

std::string HmacSha512(std::string_view key, std::string_view message,
                       OutputEncoding encoding) {
  return CalculateHmac<CryptoPP::SHA512>(key, message, encoding);
}

std::string HmacSha384(std::string_view key, std::string_view message,
                       OutputEncoding encoding) {
  return CalculateHmac<CryptoPP::SHA384>(key, message, encoding);
}

std::string HmacSha256(std::string_view key, std::string_view message,
                       OutputEncoding encoding) {
  return CalculateHmac<CryptoPP::SHA256>(key, message, encoding);
}

std::string HmacSha1(std::string_view key, std::string_view message,
                     OutputEncoding encoding) {
  return CalculateHmac<CryptoPP::SHA1>(key, message, encoding);
}

namespace weak {

std::string Md5(std::string_view data, OutputEncoding encoding) {
  return CalculateHash<CryptoPP::Weak::MD5>(data, encoding);
}

}  // namespace weak
}  // namespace crypto::hash

USERVER_NAMESPACE_END
