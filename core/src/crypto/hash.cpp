#include <crypto/hash.hpp>

#include <cryptopp/base64.h>
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

std::string EncodeString(const std::string& data,
                         crypto::hash::OutputEncoding encoding) {
  // extra case to avoid memory allocation
  if (encoding == crypto::hash::OutputEncoding::kBinary) return data;

  return EncodeArray(reinterpret_cast<const byte*>(data.c_str()), data.size(),
                     encoding);
}

template <typename HashAlgorithm>
std::string CalculateHmac(const std::string& key, const std::string& data,
                          crypto::hash::OutputEncoding encoding) {
  std::string mac;

  try {
    CryptoPP::HMAC<HashAlgorithm> hmac(
        reinterpret_cast<const byte*>(key.c_str()), key.size());
    CryptoPP::StringSource ss_key(
        data, true,
        new CryptoPP::HashFilter(hmac, new CryptoPP::StringSink(mac)));
  } catch (const CryptoPP::Exception& exc) {
    throw crypto::CryptoException(exc.what());
  }

  return EncodeString(mac, encoding);
}

template <typename HashAlgorithm>
std::string CalculateHash(const std::string& data,
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

std::string Sha1(const std::string& data, OutputEncoding encoding) {
  return CalculateHash<CryptoPP::SHA1>(data, encoding);
}

std::string Sha224(const std::string& data, OutputEncoding encoding) {
  return CalculateHash<CryptoPP::SHA224>(data, encoding);
}

std::string Sha256(const std::string& data, OutputEncoding encoding) {
  return CalculateHash<CryptoPP::SHA256>(data, encoding);
}

std::string Sha512(const std::string& data, OutputEncoding encoding) {
  return CalculateHash<CryptoPP::SHA512>(data, encoding);
}

std::string HmacSha512(const std::string& key, const std::string& message,
                       OutputEncoding encoding) {
  return CalculateHmac<CryptoPP::SHA512>(key, message, encoding);
}

std::string HmacSha256(const std::string& key, const std::string& message,
                       OutputEncoding encoding) {
  return CalculateHmac<CryptoPP::SHA256>(key, message, encoding);
}

std::string HmacSha1(const std::string& key, const std::string& message,
                     OutputEncoding encoding) {
  return CalculateHmac<CryptoPP::SHA1>(key, message, encoding);
}

namespace weak {

std::string Md5(const std::string& data, OutputEncoding encoding) {
  return CalculateHash<CryptoPP::Weak::MD5>(data, encoding);
}

}  // namespace weak
}  // namespace crypto::hash
