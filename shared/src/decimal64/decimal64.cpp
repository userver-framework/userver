#include <userver/decimal64/decimal64.hpp>

#include <string_view>

#include <fmt/format.h>

USERVER_NAMESPACE_BEGIN

namespace decimal64 {

OutOfBoundsError::OutOfBoundsError()
    : DecimalError("Out of bounds in Decimal arithmetic") {}

DivisionByZeroError::DivisionByZeroError()
    : DecimalError("Decimal divided by zero") {}

namespace impl {

namespace {

std::string_view ErrorDescription(ParseErrorCode error_code) {
  switch (error_code) {
    case ParseErrorCode::kWrongChar:
      return "wrong character";
    case ParseErrorCode::kNoDigits:
      return "the input contains no digits";
    case ParseErrorCode::kOverflow:
      return "overflow";
    case ParseErrorCode::kSpace:
      return "extra spaces are disallowed";
    case ParseErrorCode::kTrailingJunk:
      return "extra characters after the number are disallowed";
    case ParseErrorCode::kRounding:
      return "the input contains more fractional digits than in target "
             "precision, while implicit rounding is disallowed";
    case ParseErrorCode::kBoundaryDot:
      return "the input includes leading or trailing dot (like '42.'), while "
             "such notation is disallowed";
  }

  UINVARIANT(false, "Unexpected decimal64 error code");
}

}  // namespace

std::string GetErrorMessage(std::string_view source, std::string_view path,
                            size_t position, ParseErrorCode reason) {
  return fmt::format(
      "While parsing Decimal at '{}' from '{}', "
      "error at char #{}: {}",
      path, source, position + 1, ErrorDescription(reason));
}

void TrimTrailingZeros(int64_t& after, int& after_precision) {
  if (after == 0) {
    after_precision = 0;
    return;
  }
  if (after % kPow10<16> == 0) {
    after /= kPow10<16>;
    after_precision -= 16;
  }
  if (after % kPow10<8> == 0) {
    after /= kPow10<8>;
    after_precision -= 8;
  }
  if (after % kPow10<4> == 0) {
    after /= kPow10<4>;
    after_precision -= 4;
  }
  if (after % kPow10<2> == 0) {
    after /= kPow10<2>;
    after_precision -= 2;
  }
  if (after % kPow10<1> == 0) {
    after /= kPow10<1>;
    after_precision -= 1;
  }
}

std::string ToString(int64_t before, int64_t after, int precision,
                     const FormatOptions& format_options) {
  if (!format_options.is_fixed) {
    TrimTrailingZeros(after, precision);
  }

  std::string result{(after + before < 0) ? format_options.negative_sign
                                          : format_options.positive_sign};

  uint64_t groups_value[kMaxDecimalDigits];
  int groups_size[kMaxDecimalDigits];
  size_t groups_count = 0;
  char last_group = 0;
  for (const char group : format_options.grouping) {
    last_group = group;
    if (before == 0 || group <= 0 || group > kMaxDecimalDigits) {
      break;
    }
    const auto group_factor = Pow10(group);
    groups_size[groups_count] = static_cast<unsigned char>(group);
    groups_value[groups_count++] = Abs(before % group_factor);
    before /= group_factor;
  }
  if (last_group > 0 && last_group <= kMaxDecimalDigits) {
    const auto last_group_factor = Pow10(last_group);
    while (before) {
      groups_size[groups_count] = static_cast<unsigned char>(last_group);
      groups_value[groups_count++] = Abs(before % last_group_factor);
      before /= last_group_factor;
    }
  }
  if (before || groups_count == 0) {
    groups_size[groups_count] = 0;
    groups_value[groups_count++] = Abs(before);
  }
  fmt::format_to(std::back_inserter(result), FMT_COMPILE("{}"),
                 groups_value[--groups_count]);
  while (groups_count-- > 0) {
    fmt::format_to(std::back_inserter(result), FMT_COMPILE("{}{:0{}}"),
                   format_options.thousands_sep, groups_value[groups_count],
                   groups_size[groups_count]);
  }

  if (after || (format_options.is_fixed && precision)) {
    fmt::format_to(std::back_inserter(result), FMT_COMPILE("{}{:0{}}"),
                   format_options.decimal_point, Abs(after), precision);
  }
  return result;
}

}  // namespace impl

}  // namespace decimal64

USERVER_NAMESPACE_END
