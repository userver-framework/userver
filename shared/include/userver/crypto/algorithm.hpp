#pragma once

/// @file userver/crypto/algorithm.hpp
/// @brief @copybrief crypto::algorithm

#include <string_view>

/// Miscellaneous cryptographic routines
USERVER_NAMESPACE_BEGIN

namespace crypto::algorithm {

/// Performs constant-time string comparison if the strings are of equal size
bool AreStringsEqualConstTime(std::string_view str1, std::string_view str2);

}  // namespace crypto::algorithm

USERVER_NAMESPACE_END
