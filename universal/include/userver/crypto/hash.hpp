#pragma once

/// @file userver/crypto/hash.hpp
/// @brief @copybrief crypto::hash

#include <string_view>

USERVER_NAMESPACE_BEGIN

/// Cryptographic hashing
namespace crypto::hash {

enum class OutputEncoding { kBinary, kBase16, kHex = kBase16, kBase64 };

enum class Pad { kWith, kWithout };

#ifndef USERVER_NO_CRYPTOPP_BLAKE2
/// @brief Calculates Blake2-128, output format depends from encoding param
/// @param encoding result could be returned as binary string or encoded
/// @throws CryptoException internal library exception
std::string Blake2b128(std::string_view data,
                       OutputEncoding encoding = OutputEncoding::kHex);
#endif

/// @brief Calculates SHA-1, output format depends from encoding param
/// @param encoding result could be returned as binary string or encoded
/// @throws CryptoException internal library exception
std::string Sha1(std::string_view data,
                 OutputEncoding encoding = OutputEncoding::kHex);

/// @brief Calculates SHA-224, output format depends from encoding param
/// @param encoding result could be returned as binary string or encoded
/// @throws CryptoException internal library exception
std::string Sha224(std::string_view data,
                   OutputEncoding encoding = OutputEncoding::kHex);

/// @brief Calculates SHA-256, output format depends from encoding param
/// @param encoding result could be returned as binary string or encoded
/// @throws CryptoException internal library exception
std::string Sha256(std::string_view data,
                   OutputEncoding encoding = OutputEncoding::kHex);

/// @brief Calculates SHA-384, output format depends from encoding param
/// @param encoding result could be returned as binary string or encoded
/// @throws CryptoException internal library exception
std::string Sha384(std::string_view data,
                   OutputEncoding encoding = OutputEncoding::kHex);

/// @brief Calculates SHA-512, output format depends from encoding param
/// @param encoding result could be returned as binary string or encoded
/// @throws CryptoException internal library exception
std::string Sha512(std::string_view data,
                   OutputEncoding encoding = OutputEncoding::kHex);

/// @brief Calculates HMAC (using SHA-1 hash), encodes result with `encoding`
/// algorithm
/// @param key HMAC key
/// @param message data to encode
/// @param encoding result could be returned as binary string or encoded
/// @throws CryptoException internal library exception
std::string HmacSha1(std::string_view key, std::string_view message,
                     OutputEncoding encoding = OutputEncoding::kHex);

/// @brief Calculates HMAC (using SHA-256 hash), encodes result with `encoding`
/// algorithm
/// @param key HMAC key
/// @param message data to encode
/// @param encoding result could be returned as binary string or encoded
/// @throws CryptoException internal library exception
std::string HmacSha256(std::string_view key, std::string_view message,
                       OutputEncoding encoding = OutputEncoding::kHex);

/// @brief Calculates HMAC (using SHA-384 hash), encodes result with `encoding`
/// algorithm
/// @param key HMAC key
/// @param message data to encode
/// @param encoding result could be returned as binary string or encoded
/// @throws CryptoException internal library exception
std::string HmacSha384(std::string_view key, std::string_view message,
                       OutputEncoding encoding = OutputEncoding::kHex);

/// @brief Calculates HMAC (using SHA-512 hash), encodes result with `encoding`
/// algorithm
/// @param key HMAC key
/// @param message data to encode
/// @param encoding result could be returned as binary string or encoded
/// @throws CryptoException internal library exception
std::string HmacSha512(std::string_view key, std::string_view message,
                       OutputEncoding encoding = OutputEncoding::kHex);

/// Broken cryptographic hashes, must not be used except for compatibility
namespace weak {

/// @brief Calculates MD5, output format depends from encoding param
/// @param encoding result could be returned as binary string or encoded
/// @throws CryptoException internal library exception
std::string Md5(std::string_view data,
                OutputEncoding encoding = OutputEncoding::kHex);

}  // namespace weak
}  // namespace crypto::hash

USERVER_NAMESPACE_END
