#pragma once

/// @file userver/utils/str_icase.hpp
/// @brief Case insensitive ASCII comparators and hashers

#include <string>
#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @brief Case insensitive ASCII hashing functor
class StrIcaseHash {
 public:
  /// Generates a new random hash seed for each hasher instance
  StrIcaseHash();

  /// Uses the provided seed. Use with caution: a constant seed makes the hasher
  /// vulnerable to HashDOS attacks when arbitrary keys are allowed.
  explicit StrIcaseHash(std::size_t seed) noexcept;

  std::size_t operator()(std::string_view s) const& noexcept;

 private:
  std::size_t seed_;
};

/// Case insensitive ASCII 3-way comparison functor
class StrIcaseCompareThreeWay {
 public:
  // TODO: std::weak_ordering
  /// @returns integer <0 when `lhs < rhs`, >0 when `lhs > rhs` and 0 otherwise
  /// @{
  int operator()(const std::string& lhs, const std::string& rhs) const
      noexcept {
    return (*this)(std::string_view{lhs}, std::string_view{rhs});
  }

  int operator()(std::string_view lhs, std::string_view rhs) const noexcept;
  /// @}
};

/// Case insensitive ASCII equality comparison functor
class StrIcaseEqual {
 public:
  bool operator()(const std::string& lhs, const std::string& rhs) const
      noexcept {
    return (*this)(std::string_view{lhs}, std::string_view{rhs});
  }

  bool operator()(std::string_view lhs, std::string_view rhs) const noexcept;
};

/// Case insensitive ASCII less comparison functor
class StrIcaseLess {
 public:
  bool operator()(const std::string& lhs, const std::string& rhs) const
      noexcept {
    return (*this)(std::string_view{lhs}, std::string_view{rhs});
  }

  bool operator()(std::string_view lhs, std::string_view rhs) const noexcept;
};

}  // namespace utils

USERVER_NAMESPACE_END
