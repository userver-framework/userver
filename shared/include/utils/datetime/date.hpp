#pragma once

#include <chrono>
#include <iosfwd>
#include <stdexcept>
#include <string>

#include <utils/strong_typedef.hpp>

#include <formats/common/meta.hpp>

namespace logging {
class LogHelper;
}

namespace utils::datetime {

/// Date in format YYYY-MM-DD. Convertible to
/// std::chrono::system_clock::time_point and could be constructed from year,
/// month and day integers.
struct Date final
    : utils::StrongTypedef<Date, std::chrono::system_clock::time_point> {
  using StrongTypedef::StrongTypedef;

  /// Constructs Date without validation of input arguments
  Date(int year, int month, int day);
};

/// Validates date_string and constructs date from YYYY-MM-DD string and
Date DateFromRFC3339String(const std::string& date_string);

/// Outputs date as a YYYY-MM-DD string
std::string ToString(Date date);

template <typename Value>
std::enable_if_t<formats::common::kIsFormatValue<Value>, Date> Parse(
    const Value& value, formats::parse::To<Date>) {
  std::string str;
  try {
    str = value.template As<std::string>();
  } catch (const std::exception& e) {
    throw typename Value::ParseException(
        "Only strings can be parsed as `utils::datetime::Date`");
  }

  try {
    return utils::datetime::DateFromRFC3339String(str);
  } catch (const std::exception& e) {
    throw typename Value::ParseException(
        "'" + str + "' cannot be parsed to `utils::datetime::Date`");
  }
}

template <typename Value>
std::enable_if_t<formats::common::kIsFormatValue<Value>, Value> Serialize(
    Date date, formats::serialize::To<Value>) {
  return typename Value::Builder(ToString(date)).ExtractValue();
}

template <typename StringBuilder>
void WriteToStream(Date value, StringBuilder& sw) {
  WriteToStream(ToString(value), sw);
}

template <class LogHelper>
std::enable_if_t<std::is_same_v<LogHelper, logging::LogHelper>, LogHelper&>
operator<<(LogHelper& lh, Date date) {
  // This function was made template to work well with forward
  // declared logging::LogHelper

  return lh << ToString(date);
}

std::ostream& operator<<(std::ostream& os, Date date);

}  // namespace utils::datetime
