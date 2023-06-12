#pragma once

/// @file userver/formats/parse/common.hpp
/// @brief Parsers and converters for std::chrono::seconds,
/// std::chrono::system_clock::time_point and integral types
///
/// @ingroup userver_formats_parse

#include <chrono>
#include <cstdint>
#include <limits>

#include <fmt/format.h>

#include <userver/formats/common/meta.hpp>
#include <userver/formats/parse/to.hpp>
#include <userver/utils/datetime.hpp>
#include <userver/utils/datetime/from_string_saturating.hpp>
#include <userver/utils/meta.hpp>
#include <userver/utils/string_to_duration.hpp>

USERVER_NAMESPACE_BEGIN

/// Generic parsers and converters
namespace formats::parse {
namespace impl {

template <typename T, typename Value>
void CheckInBounds(const Value& value, T x, T min, T max) {
  if (x < min || x > max) {
    throw typename Value::ParseException(
        fmt::format("Value of '{}' is out of bounds ({} <= {} <= {})",
                    value.GetPath(), min, x, max));
  }
}

template <typename Value>
float NarrowToFloat(double x, const Value& value) {
  CheckInBounds<double>(value, x, std::numeric_limits<float>::lowest(),
                        std::numeric_limits<float>::max());
  return static_cast<float>(x);
}

template <typename Dst, typename Value, typename Src>
Dst NarrowToInt(Src x, const Value& value) {
  static_assert(
      std::numeric_limits<Src>::min() <= std::numeric_limits<Dst>::min() &&
          std::numeric_limits<Src>::max() >= std::numeric_limits<Dst>::max(),
      "expanding cast requested");

  CheckInBounds<Src>(value, x, std::numeric_limits<Dst>::min(),
                     std::numeric_limits<Dst>::max());
  return static_cast<Dst>(x);
}

template <typename Value>
std::chrono::seconds ToSeconds(const std::string& data, const Value& value) {
  const auto ms = utils::StringToDuration(data);
  const auto converted = std::chrono::duration_cast<std::chrono::seconds>(ms);
  if (converted != ms) {
    throw typename Value::ParseException(
        fmt::format("Value of '{}' = {}ms cannot be represented as "
                    "'std::chrono::seconds' without precision loss",
                    value.GetPath(), ms.count()));
  }
  return converted;
}

}  // namespace impl

template <typename Value>
float Parse(const Value& value, To<float>) {
  return impl::NarrowToFloat(value.template As<double>(), value);
}

template <typename Value, typename T>
std::enable_if_t<common::kIsFormatValue<Value> && meta::kIsInteger<T>, T> Parse(
    const Value& value, To<T>) {
  using IntT = std::conditional_t<std::is_signed<T>::value, int64_t, uint64_t>;
  return impl::NarrowToInt<T>(value.template As<IntT>(), value);
}

template <typename Value, typename Period>
std::enable_if_t<common::kIsFormatValue<Value>,
                 std::chrono::duration<double, Period>>
Parse(const Value& n, To<std::chrono::duration<double, Period>>) {
  return std::chrono::duration<double, Period>(n.template As<double>());
}

template <typename Value>
std::enable_if_t<common::kIsFormatValue<Value>, std::chrono::seconds> Parse(
    const Value& n, To<std::chrono::seconds>) {
  return n.IsInt64() ? std::chrono::seconds{n.template As<int64_t>()}
                     : impl::ToSeconds(n.template As<std::string>(), n);
}

template <class Value>
std::chrono::system_clock::time_point Parse(
    const Value& n, To<std::chrono::system_clock::time_point>) {
  return utils::datetime::FromRfc3339StringSaturating(
      n.template As<std::string>());
}

template <class Value>
float Convert(const Value& value, To<float>) {
  return impl::NarrowToFloat(value.template ConvertTo<double>(), value);
}

template <typename Value, typename T>
std::enable_if_t<meta::kIsInteger<T>, T> Convert(const Value& value, To<T>) {
  using IntT = std::conditional_t<std::is_signed<T>::value, int64_t, uint64_t>;
  return impl::NarrowToInt<T>(value.template ConvertTo<IntT>(), value);
}

template <typename Value>
std::chrono::seconds Convert(const Value& n, To<std::chrono::seconds>) {
  return n.IsInt64() ? std::chrono::seconds{n.template ConvertTo<int64_t>()}
                     : impl::ToSeconds(n.template ConvertTo<std::string>(), n);
}

}  // namespace formats::parse

USERVER_NAMESPACE_END
