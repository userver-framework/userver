#pragma once

/// @file userver/crypto/base64.hpp
/// @brief @copybrief crypto::base64
/// @ingroup userver_universal

#include <string>
#include <string_view>

USERVER_NAMESPACE_BEGIN

/// @brief Cryptographic hashing, base64 encoding and decoding helpers.
namespace crypto::base64 {

enum class Pad { kWith, kWithout };

/// @brief Encodes data to Base64, add padding by default
/// @param data binary data to encode
/// @param pad controls if pad should be added or not
/// @throws CryptoException internal library exception
std::string Base64Encode(std::string_view data, Pad pad = Pad::kWith);

/// @brief Decodes data from Base64, padding is optional, line breaks are skipped, unused bits of the last character
/// are ignored
/// @throws CryptoException if @a data contains other characters outside of the alphabet (including spaces and
/// the URL alphabet), '=' not at the end, more than two '=' or if the number of characters before '=' is 1 modulo 4
std::string Base64Decode(std::string_view data);

#ifndef USERVER_NO_CRYPTOPP_BASE64_URL

/// @brief Encodes data to Base64 (using URL alphabet), add padding by default
/// @param data binary data to encode
/// @param pad controls if pad should be added or not
/// @throws CryptoException internal library exception
std::string Base64UrlEncode(std::string_view data, Pad pad = Pad::kWith);

/// @brief Decodes data from Base64 (using URL alphabet), padding is optional, line breaks are skipped, unused bits
/// of the last character are ignored
/// @throws CryptoException if @a data contains other characters outside of the URL alphabet (including spaces and
/// the standard alphabet), '=' not at the end, more than two '=' or if the number of characters before '=' is
/// 1 modulo 4
std::string Base64UrlDecode(std::string_view data);

/// @brief Decodes @a data in-place and returns `true` on success.
/// @note Supports both traditional and "web-safe" base64 encodings.
[[nodiscard]] bool Base64UniversalDecodeInPlace(std::string& data) noexcept;

#endif

}  // namespace crypto::base64

USERVER_NAMESPACE_END
