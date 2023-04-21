#pragma once

#include <string>

#include <userver/formats/parse/to.hpp>
#include <userver/formats/serialize/to.hpp>
#include <userver/utils/small_string.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

template <typename Value, std::size_t N>
Value Serialize(const SmallString<N>& value, formats::serialize::To<Value>) {
  return typename Value::Builder{std::string_view{value}}.ExtractValue();
}

template <typename Value, std::size_t N>
SmallString<N> Parse(const Value& value, formats::parse::To<SmallString<N>>) {
  return SmallString<N>{value.template As<std::string>()};
}

}  // namespace utils

USERVER_NAMESPACE_END
