#pragma once

/// @file crypto/algorithm.hpp
/// @brief @copybrief crypto::algorithm

#include <utils/string_view.hpp>

/// Miscellaneous cryptographic routines
namespace crypto::algorithm {

/// Performs constant-time string comparison if the strings are of equal size
bool AreStringsEqualConstTime(utils::string_view str1, utils::string_view str2);

}  // namespace crypto::algorithm
