#pragma once

/// @file userver/formats/parse/boost_uuid.hpp
/// @brief boost::uuids::uuid parser for any format
/// @ingroup userver_formats_parse

#include <optional>
#include <string>

#include <boost/uuid/uuid.hpp>

#include <userver/formats/common/meta.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/utils/boost_uuid4.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::parse {

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
  std::optional<std::string> str;
  try {
    str = value.template As<std::string>();
    return utils::BoostUuidFromString(*str);
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

USERVER_NAMESPACE_END
