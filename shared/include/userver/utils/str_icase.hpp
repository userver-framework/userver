#pragma once

/// @file userver/utils/str_icase.hpp
/// @brief Case insensitive ASCII comparators and hashers

#include <string>
#include <string_view>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// The seed structure used by underlying hashing implementation
struct HashSeed final {
  std::uint64_t k0;
  std::uint64_t k1;
};

/// @brief Case sensitive ASCII hashing functor
class StrCaseHash {
 public:
  /// Generates a new random hash seed for each hasher instance
  StrCaseHash();

  /// Uses the provided seed. Use with caution: a constant seed makes the hasher
  /// vulnerable to HashDOS attacks when arbitrary keys are allowed.
  explicit StrCaseHash(HashSeed seed) noexcept;

  std::size_t operator()(std::string_view s) const& noexcept;

  template <class StringStrongTypedef>
  auto operator()(const StringStrongTypedef& s) const& noexcept
      -> decltype(operator()(std::string_view{s.GetUnderlying()})) {
    static_assert(noexcept((*this)(std::string_view{s.GetUnderlying()})),
                  "GetUnderlying() should not throw as this affects efficiency "
                  "on some platforms");

    return (*this)(std::string_view{s.GetUnderlying()});
  }

  HashSeed GetSeed() const noexcept { return seed_; }

 private:
  HashSeed seed_;
};

/// @brief Case insensitive ASCII hashing functor
class StrIcaseHash {
 public:
  /// Generates a new random hash seed for each hasher instance
  StrIcaseHash();

  /// Uses the provided seed. Use with caution: a constant seed makes the hasher
  /// vulnerable to HashDOS attacks when arbitrary keys are allowed.
  explicit StrIcaseHash(HashSeed seed) noexcept;

  std::size_t operator()(std::string_view s) const& noexcept;

 private:
  HashSeed seed_;
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
