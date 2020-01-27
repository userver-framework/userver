#pragma once

/// @file formats/parse/boost_uuid.hpp
/// @brief boost::uuids::uuid parser for any format

#include <string>

#include <boost/optional.hpp>
#include <boost/uuid/uuid.hpp>

#include <formats/common/meta.hpp>
#include <formats/parse/to.hpp>

namespace formats::parse {

namespace detail {

boost::uuids::uuid ParseString(std::string const& str);

}  // namespace detail

/**
 * Valid uuid strings:
 * 0123456789abcdef0123456789abcdef
 * 01234567-89ab-cdef-0123-456789abcdef
 * {01234567-89ab-cdef-0123-456789abcdef}
 * {0123456789abcdef0123456789abcdef}
 */
template <typename Value>
std::enable_if_t<common::kIsFormatValue<Value>, boost::uuids::uuid> Parse(
    const Value& value, To<boost::uuids::uuid>) {
  boost::optional<std::string> str;
  try {
    str = value.template As<std::string>();
    return detail::ParseString(*str);
  } catch (const std::exception& e) {
    if (!!str) {
      throw typename Value::ParseException(
          "'" + *str + "' cannot be parsed to `boost::uuids::uuid`");
    } else {
      throw typename Value::ParseException(
          "Only strings can be parsed as boost uuid");
    }
  }
}

}  // namespace formats::parse
