#pragma once

/// @file crypto/base64.hpp
/// @brief @copybrief crypto::base64

#include <utils/string_view.hpp>

/// Cryptographic hashing
namespace crypto::base64 {

enum class Pad { kWith, kWithout };

/// @brief Encodes data to Base64, add padding by default
/// @param pad controls if pad should be added or not
/// @throws CryptoException internal library exception
std::string Base64Encode(utils::string_view data, Pad pad = Pad::kWith);

/// @brief Decodes data from Base64
/// @throws CryptoException internal library exception
std::string Base64Decode(utils::string_view data);

/// @brief Encodes data to Base64 (using URL alphabet), add padding by default
/// @param pad controls if pad should be added or not
/// @throws CryptoException internal library exception
std::string Base64UrlEncode(utils::string_view data, Pad pad = Pad::kWith);

/// @brief Decodes data from Base64 (using URL alphabet)
/// @throws CryptoException internal library exception
std::string Base64UrlDecode(utils::string_view data);

}  // namespace crypto::base64
