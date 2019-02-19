#pragma once

/// @file crypto/hash.hpp
/// @brief @copybrief crypto::hash

#include <string>

/// Cryptographic hashing
namespace crypto::hash {

/// Calculates lowercase hex-encoded SHA-256 hash of the data
std::string Sha256(const std::string& data);

/// Calculates lowercase hex-encoded SHA-512 hash of the data
std::string Sha512(const std::string& data);

/// Broken cryptographic hashes, must not be used except for compatibility
namespace weak {

/// Calculates lowercase hex-encoded MD5 hash of the data
std::string Md5(const std::string& data);

}  // namespace weak
}  // namespace crypto::hash
