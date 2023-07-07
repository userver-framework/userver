#pragma once

/// @file userver/decimal64/format_options.hpp
/// @brief Structure storing format options for decimal

#include <optional>
#include <string>

USERVER_NAMESPACE_BEGIN

namespace decimal64 {

/// @brief Parameters for flexible decimal formatting
struct FormatOptions {
  /// separator of fractional and integer parts
  std::string decimal_point = ".";

  /// separator of groups in the integer part
  std::string thousands_sep;

  /// rule of grouping in the integer part
  /// https://en.cppreference.com/w/cpp/locale/numpunct/grouping
  std::string grouping;

  /// for negative numbers, the specified string will be inserted at beginning
  std::string negative_sign = "-";

  /// for non-negative(>=0) numbers will be inserted at beginning
  std::string positive_sign;

  /// maximum number of digits in the fractional part
  /// if `nullopt`, then the value is taken from the template argument
  std::optional<int> precision;

  /// if true, then the fractional part will have the maximum number of digits
  bool is_fixed = false;
};

}  // namespace decimal64

USERVER_NAMESPACE_END
