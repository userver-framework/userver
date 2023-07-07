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
bool AreStringsEqualConstTime(std::string_view str1,
                              std::string_view str2) noexcept;

/// Performs constant-time string comparison comparator
struct StringsEqualConstTimeComparator {
  bool operator()(std::string_view x, std::string_view y) const noexcept {
    return algorithm::AreStringsEqualConstTime(x, y);
  }

  /// Overload for utils::StrongTypedef that hold string like objects
  template <typename T>
  auto operator()(const T& x, const T& y) const noexcept
      -> decltype(algorithm::AreStringsEqualConstTime(x.GetUnderlying(),
                                                      y.GetUnderlying())) {
    static_assert(noexcept(algorithm::AreStringsEqualConstTime(
                      x.GetUnderlying(), y.GetUnderlying())),
                  "The comparator should not throw as this affects efficiency "
                  "on some platforms");

    return algorithm::AreStringsEqualConstTime(x.GetUnderlying(),
                                               y.GetUnderlying());
  }
};

}  // namespace crypto::algorithm

USERVER_NAMESPACE_END
