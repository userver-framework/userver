#pragma once

#include <formats/parse/to.hpp>

#include <chrono>
#include <limits>

#include <utils/demangle.hpp>
#include <utils/string_to_duration.hpp>

namespace formats::parse {

template <class Value>
float Parse(const Value& value, To<float>) {
  auto val = value.template As<double>();
  auto min = std::numeric_limits<float>::lowest();
  auto max = std::numeric_limits<float>::max();
  if (val < min || val > max)
    throw typename Value::ParseException(
        std::string("Value of '") + value.GetPath() + "' is out of bounds (" +
        std::to_string(min) + " <= " + std::to_string(val) +
        " <= " + std::to_string(max) + ")");

  return static_cast<float>(val);
}

template <class Value, typename T>
std::enable_if_t<std::is_integral<T>::value && (sizeof(T) > 1), T> Parse(
    const Value& value, To<T>) {
  using IntT = std::conditional_t<std::is_signed<T>::value, int64_t, uint64_t>;

  auto val = value.template As<IntT>();
  auto min = static_cast<IntT>(std::numeric_limits<T>::min());
  auto max = static_cast<IntT>(std::numeric_limits<T>::max());
  if (val < min || val > max)
    throw typename Value::ParseException(
        std::string("Value of '") + value.GetPath() + "' is out of bounds (" +
        std::to_string(min) + " <= " + std::to_string(val) +
        " <= " + std::to_string(max) + ")");

  return val;
}

template <class Value>
auto Parse(const Value& n, To<std::chrono::seconds>) {
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
        utils::GetTypeName<std::chrono::seconds>() + " without precision loss");
  }

  return to;
}

}  // namespace formats::parse
