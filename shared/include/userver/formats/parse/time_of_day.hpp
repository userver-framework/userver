#pragma once

/// @file userver/formats/parse/time_of_day.hpp
/// @brief utils::datetime::TimeOfDay parser for any format
///
/// @ingroup userver_formats_parse

#include <userver/utils/time_of_day.hpp>

#include <userver/formats/common/meta.hpp>
#include <userver/formats/parse/to.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::parse {

template <typename Value, typename Duration>
std::enable_if_t<common::kIsFormatValue<Value>,
                 utils::datetime::TimeOfDay<Duration>>
Parse(const Value& value, To<utils::datetime::TimeOfDay<Duration>>) {
  std::optional<std::string> str;
  try {
    str = value.template As<std::string>();
    return utils::datetime::TimeOfDay<Duration>{*str};
  } catch (const std::exception& e) {
    if (str) {
      throw typename Value::ParseException(
          "'" + *str + "' cannot be parsed to `utils::datetime::TimeOfDay`");
    } else {
      throw typename Value::ParseException(
          "Only strings can be parsed as `utils::datetime::TimeOfDay`");
    }
  }
}

}  // namespace formats::parse

USERVER_NAMESPACE_END
