#pragma once

/// @file userver/crypto/algorithm.hpp
/// @brief @copybrief crypto::algorithm

#include <string_view>

USERVER_NAMESPACE_BEGIN

/// Miscellaneous cryptographic routines
namespace crypto::algorithm {

/// Performs constant-time string comparison if the strings are of equal size
///
/// @snippet storages/secdist/secdist_test.cpp UserPasswords
bool AreStringsEqualConstTime(std::string_view str1, std::string_view str2);

}  // namespace crypto::algorithm

USERVER_NAMESPACE_END
