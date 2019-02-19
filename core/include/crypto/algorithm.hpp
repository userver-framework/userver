#pragma once

/// @file crypto/algorithm.hpp
/// @brief @copybrief crypto::algorithm

#include <string>

/// Miscellaneous cryptographic routines
namespace crypto::algorithm {

/// Performs constant-time string comparison if the strings are of equal size
bool AreStringsEqualConstTime(const std::string& str1, const std::string& str2);

}  // namespace crypto::algorithm
