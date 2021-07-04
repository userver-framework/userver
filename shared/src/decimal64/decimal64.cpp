#include <userver/decimal64/decimal64.hpp>

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
}

}  // namespace

std::string GetErrorMessage(std::string_view source, std::string_view path,
                            size_t position, ParseErrorCode reason) {
  return fmt::format(FMT_STRING("While parsing Decimal at '{}' from '{}', "
                                "error at char #{}: {}"),
                     path, source, position + 1, ErrorDescription(reason));
}

}  // namespace impl

}  // namespace decimal64
