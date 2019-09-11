#pragma once

#include <chrono>
#include <cstdint>
#include <limits>

#include <boost/numeric/conversion/cast.hpp>

#include <compiler/demangle.hpp>
#include <formats/common/meta.hpp>
#include <formats/parse/to.hpp>
#include <utils/datetime.hpp>
#include <utils/string_to_duration.hpp>

namespace formats::parse {
namespace impl {

template <class Value>
float NarrowToFloat(double x, const Value& value) {
  auto min = std::numeric_limits<float>::lowest();
  auto max = std::numeric_limits<float>::max();
  if (x < min || x > max)
    throw typename Value::ParseException(
        std::string("Value of '") + value.GetPath() + "' is out of bounds (" +
        std::to_string(min) + " <= " + std::to_string(x) +
        " <= " + std::to_string(max) + ")");

  return static_cast<float>(x);
}

template <typename Dst, class Value, typename Src>
Dst NarrowToInt(Src x, const Value& value) {
  static_assert(
      std::numeric_limits<Src>::min() <= std::numeric_limits<Dst>::min() &&
          std::numeric_limits<Src>::max() >= std::numeric_limits<Dst>::max(),
      "expanding cast requested");

  auto min = static_cast<Src>(std::numeric_limits<Dst>::min());
  auto max = static_cast<Src>(std::numeric_limits<Dst>::max());
  if (x < min || x > max)
    throw typename Value::ParseException(
        std::string("Value of '") + value.GetPath() + "' is out of bounds (" +
        std::to_string(min) + " <= " + std::to_string(x) +
        " <= " + std::to_string(max) + ")");

  return x;
}

}  // namespace impl

template <class Value>
float Parse(const Value& value, To<float>) {
  return impl::NarrowToFloat(value.template As<double>(), value);
}

template <class Value, typename T>
std::enable_if_t<common::kIsFormatValue<Value> && std::is_integral<T>::value &&
                     (sizeof(T) > 1),
                 T>
Parse(const Value& value, To<T>) {
  using IntT = std::conditional_t<std::is_signed<T>::value, int64_t, uint64_t>;
  return impl::NarrowToInt<T>(value.template As<IntT>(), value);
}

template <class Value>
std::enable_if_t<common::kIsFormatValue<Value>, std::chrono::seconds> Parse(
    const Value& n, To<std::chrono::seconds>) {
  std::chrono::seconds to;
  bool succeeded;
  if (n.IsInt64()) {
    const auto dur = n.template As<std::int64_t>();
    to = std::chrono::seconds{dur};
    succeeded = (to.count() == dur);
  } else {
    const auto dur = utils::StringToDuration(n.template As<std::string>());
    to = std::chrono::duration_cast<std::chrono::seconds>(dur);
    succeeded = (to == dur);
  }

  if (!succeeded) {
    throw typename Value::ParseException(
        "'" + n.template As<std::string>() + "' can not be represented as " +
        compiler::GetTypeName<std::chrono::seconds>() +
        " without precision loss");
  }

  return to;
}

template <typename Rep, typename Period>
std::chrono::duration<Rep, Period> Parse(
    std::int64_t n, To<std::chrono::duration<Rep, Period>>) {
  return std::chrono::duration<Rep, Period>{n};
}

/* signed <=> unsigned */
template <typename T, typename U>
std::enable_if_t<std::is_integral<T>::value && std::is_integral<U>::value &&
                     sizeof(T) == sizeof(U),
                 U>
Parse(T n, To<U>) {
  return boost::numeric_cast<U>(n);
}

template <class Value>
std::chrono::system_clock::time_point Parse(
    const Value& n, To<std::chrono::system_clock::time_point>) {
  return utils::datetime::Stringtime(n.template As<std::string>(), "UTC",
                                     utils::datetime::kRfc3339Format);
}

template <class Value>
float Convert(const Value& value, To<float>) {
  return impl::NarrowToFloat(value.template ConvertTo<double>(), value);
}

template <class Value, typename T>
std::enable_if_t<std::is_integral<T>::value && (sizeof(T) > 1), T> Convert(
    const Value& value, To<T>) {
  using IntT = std::conditional_t<std::is_signed<T>::value, int64_t, uint64_t>;
  return impl::NarrowToInt<T>(value.template ConvertTo<IntT>(), value);
}

template <class Value>
auto Convert(const Value& n, To<std::chrono::seconds>) {
  std::chrono::seconds to;
  bool succeeded;
  if (n.IsInt64()) {
    const auto dur = n.template ConvertTo<std::int64_t>();
    to = std::chrono::seconds{dur};
    succeeded = (to.count() == dur);
  } else {
    const auto dur =
        utils::StringToDuration(n.template ConvertTo<std::string>());
    to = std::chrono::duration_cast<std::chrono::seconds>(dur);
    succeeded = (to == dur);
  }

  if (!succeeded) {
    throw typename Value::ParseException(
        "'" + n.template As<std::string>() + "' can not be represented as " +
        compiler::GetTypeName<std::chrono::seconds>() +
        " without precision loss");
  }

  return to;
}

}  // namespace formats::parse
