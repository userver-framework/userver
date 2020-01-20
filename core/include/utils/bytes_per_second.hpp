#pragma once

#include <string>

#include <formats/parse/to.hpp>

namespace utils {

/// Data type that represents `bytes per second` unit
enum class BytesPerSecond : long long {};

constexpr long long ToLongLong(BytesPerSecond x) {
  return static_cast<long long>(x);
}

constexpr bool operator==(BytesPerSecond lhs, BytesPerSecond rhs) {
  return ToLongLong(lhs) == ToLongLong(rhs);
}

constexpr bool operator!=(BytesPerSecond lhs, BytesPerSecond rhs) {
  return !(lhs == rhs);
}

/// Understands all the date-rate unit suffixes from
/// https://en.wikipedia.org/wiki/Data-rate_units#Conversion_table
BytesPerSecond StringToBytesPerSecond(const std::string& data);

template <class Value>
BytesPerSecond Parse(const Value& v, formats::parse::To<BytesPerSecond>) {
  return StringToBytesPerSecond(v.template As<std::string>());
}

}  // namespace utils
