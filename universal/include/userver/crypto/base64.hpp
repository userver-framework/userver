#pragma once

/// @file userver/crypto/base64.hpp
/// @brief @copybrief crypto::base64
/// @ingroup userver_universal

#include <string>
#include <string_view>

USERVER_NAMESPACE_BEGIN

/// Cryptographic hashing
namespace crypto::base64 {

enum class Pad { kWith, kWithout };

/// @brief Encodes data to Base64, add padding by default
/// @param data binary data to encode
/// @param pad controls if pad should be added or not
/// @throws CryptoException internal library exception
std::string Base64Encode(std::string_view data, Pad pad = Pad::kWith);

/// @brief Decodes data from Base64
/// @throws CryptoException internal library exception
std::string Base64Decode(std::string_view data);

#ifndef USERVER_NO_CRYPTOPP_BASE64_URL

/// @brief Encodes data to Base64 (using URL alphabet), add padding by default
/// @param data binary data to encode
/// @param pad controls if pad should be added or not
/// @throws CryptoException internal library exception
std::string Base64UrlEncode(std::string_view data, Pad pad = Pad::kWith);

/// @brief Decodes data from Base64 (using URL alphabet)
/// @throws CryptoException internal library exception
std::string Base64UrlDecode(std::string_view data);

/// @brief Decodes @a data in-place and returns `true` on success.
/// @note Supports both traditional and "web-safe" base64 encodings.
[[nodiscard]] bool Base64UniversalDecodeInPlace(std::string& data) noexcept;

#endif

}  // namespace crypto::base64

USERVER_NAMESPACE_END
