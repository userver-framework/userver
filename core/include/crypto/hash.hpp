#pragma once

/// @file crypto/hash.hpp
/// @brief @copybrief crypto::hash

#include <string>

/// Cryptographic hashing
namespace crypto::hash {

enum class OutputEncoding { kBinary, kBase16, kHex = kBase16, kBase64 };

enum class Pad { kWith, kWithout };

/// @brief Calculates Blake2-128, output format depends from encoding param
/// @param encoding result could be returned as binary string or encoded
/// @throws CryptoException internal library exception
std::string Blake2b128(const std::string& data,
                       OutputEncoding encoding = OutputEncoding::kHex);

/// @brief Calculates SHA-1, output format depends from encoding param
/// @param encoding result could be returned as binary string or encoded
/// @throws CryptoException internal library exception
std::string Sha1(const std::string& data,
                 OutputEncoding encoding = OutputEncoding::kHex);

/// @brief Calculates SHA-224, output format depends from encoding param
/// @param encoding result could be returned as binary string or encoded
/// @throws CryptoException internal library exception
std::string Sha224(const std::string& data,
                   OutputEncoding encoding = OutputEncoding::kHex);

/// @brief Calculates SHA-256, output format depends from encoding param
/// @param encoding result could be returned as binary string or encoded
/// @throws CryptoException internal library exception
std::string Sha256(const std::string& data,
                   OutputEncoding encoding = OutputEncoding::kHex);

/// @brief Calculates SHA-512, output format depends from encoding param
/// @param encoding result could be returned as binary string or encoded
/// @throws CryptoException internal library exception
std::string Sha512(const std::string& data,
                   OutputEncoding encoding = OutputEncoding::kHex);

/// @brief Calculates HMAC (using SHA-1 hash), encodes result with `encoding`
/// algorithm
/// @param key HMAC key
/// @param message data to encode
/// @param encoding result could be returned as binary string or encoded
/// @throws CryptoException internal library exception
std::string HmacSha1(const std::string& key, const std::string& message,
                     OutputEncoding encoding = OutputEncoding::kHex);

/// @brief Calculates HMAC (using SHA-256 hash), encodes result with `encoding`
/// algorithm
/// @param key HMAC key
/// @param message data to encode
/// @param encoding result could be returned as binary string or encoded
/// @throws CryptoException internal library exception
std::string HmacSha256(const std::string& key, const std::string& message,
                       OutputEncoding encoding = OutputEncoding::kHex);

/// @brief Calculates HMAC (using SHA-512 hash), encodes result with `encoding`
/// algorithm
/// @param key HMAC key
/// @param message data to encode
/// @param encoding result could be returned as binary string or encoded
/// @throws CryptoException internal library exception
std::string HmacSha512(const std::string& key, const std::string& message,
                       OutputEncoding encoding = OutputEncoding::kHex);

/// Base64

/// @brief Encodes data to Base64, add padding by default
/// @param pad controls if pad should be added or not
/// @throws CryptoException internal library exception
std::string Base64Encode(const std::string& data, Pad pad = Pad::kWith);

/// @brief Decodes data from Base64
/// @throws CryptoException internal library exception
std::string Base64Decode(const std::string& data);

/// Broken cryptographic hashes, must not be used except for compatibility
namespace weak {

/// @brief Calculates MD5, output format depends from encoding param
/// @param encoding result could be returned as binary string or encoded
/// @throws CryptoException internal library exception
std::string Md5(const std::string& data,
                OutputEncoding encoding = OutputEncoding::kHex);

}  // namespace weak
}  // namespace crypto::hash
