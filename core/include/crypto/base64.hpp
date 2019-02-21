#pragma once

/// @file crypto/base64.hpp
/// @brief @copybrief crypto::base64

#include <string>

/// Cryptographic hashing
namespace crypto::base64 {

enum class Pad { kWith, kWithout };

/// @brief Encodes data to Base64, add padding by default
/// @param pad controls if pad should be added or not
/// @throws CryptoException internal library exception
std::string Base64Encode(const std::string& data, Pad pad = Pad::kWith);

/// @brief Decodes data from Base64
/// @throws CryptoException internal library exception
std::string Base64Decode(const std::string& data);

}  // namespace crypto::base64
