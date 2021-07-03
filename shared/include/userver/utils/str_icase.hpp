#pragma once

/// @file utils/str_icase.hpp
/// @brief Case insensitive ASCII comparators and hashers

#include <string>
#include <string_view>

namespace utils {

/// Case insensitive ASCII hashing functor
class StrIcaseHash {
 public:
  std::size_t operator()(const std::string& s) const noexcept {
    return (*this)(std::string_view(s));
  }

  std::size_t operator()(std::string_view s) const noexcept;
};

/// Case insensitive ASCII 3-way comparison functor
class StrIcaseCompareThreeWay {
 public:
  // TODO: std::weak_ordering
  /// @returns integer <0 when `lhs < rhs`, >0 when `lhs > rhs` and 0 otherwise
  /// @{
  int operator()(const std::string& lhs, const std::string& rhs) const
      noexcept {
    return (*this)(std::string_view(lhs), std::string_view(rhs));
  }

  int operator()(std::string_view lhs, std::string_view rhs) const noexcept;
  /// @}
};

/// Case insensitive ASCII equality comparison functor
class StrIcaseEqual {
 public:
  bool operator()(const std::string& lhs, const std::string& rhs) const
      noexcept {
    return (*this)(std::string_view(lhs), std::string_view(rhs));
  }

  bool operator()(std::string_view lhs, std::string_view rhs) const noexcept;
};

/// Case insensitive ASCII less comparison functor
class StrIcaseLess {
 public:
  bool operator()(const std::string& lhs, const std::string& rhs) const
      noexcept {
    return (*this)(std::string_view(lhs), std::string_view(rhs));
  }

  bool operator()(std::string_view lhs, std::string_view rhs) const noexcept;
};

}  // namespace utils
