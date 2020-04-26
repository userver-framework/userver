#pragma once

#include <string>
#include <string_view>

namespace utils {

class StrIcaseHash final {
 public:
  std::size_t operator()(const std::string& s) const;
};

class StrIcaseCompareThreeWay final {
 public:
  // TODO: std::weak_ordering
  /// @returns integer <0 when `lhs < rhs`, >0 when `lhs > rhs` and 0 otherwise
  /// @{
  int operator()(const std::string& lhs, const std::string& rhs) const {
    return (*this)(std::string_view(lhs), std::string_view(rhs));
  }

  int operator()(std::string_view lhs, std::string_view rhs) const;
  /// @}
};

class StrIcaseEqual final {
 public:
  bool operator()(const std::string& lhs, const std::string& rhs) const {
    return (*this)(std::string_view(lhs), std::string_view(rhs));
  }

  bool operator()(std::string_view lhs, std::string_view rhs) const;

 private:
  StrIcaseCompareThreeWay icase_cmp_;
};

class StrIcaseLess final {
 public:
  bool operator()(const std::string& lhs, const std::string& rhs) const {
    return (*this)(std::string_view(lhs), std::string_view(rhs));
  }

  bool operator()(std::string_view lhs, std::string_view rhs) const;

 private:
  StrIcaseCompareThreeWay icase_cmp_;
};

}  // namespace utils
