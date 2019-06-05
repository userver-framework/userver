#pragma once

#include <iosfwd>
#include <utility>

#include <formats/parse/to.hpp>

namespace logging {
class LogHelper;
}

namespace utils {

/// Helper class for data that MUST NOT be logged or outputted in some other
/// way.
template <class T>
struct NonLoggable {
  explicit NonLoggable(T&& value) : data_(std::move(value)) {}

  const T& GetUnprotectedRawValue() const noexcept { return data_; }

 private:
  T data_;
};

template <class Char, class Triats, class T>
std::basic_ostream<Char, Triats>& operator<<(
    std::basic_ostream<Char, Triats>& os, NonLoggable<T>) {
  static_assert(!sizeof(NonLoggable<T>),
                "Data in NonLoggable MUST NOT be logged or "
                "outputted in some other way");

  return os;
}

template <class T>
logging::LogHelper& operator<<(logging::LogHelper& os, NonLoggable<T>) {
  static_assert(!sizeof(NonLoggable<T>),
                "Data in NonLoggabl MUST NOT be logged or "
                "outputted in some other way");

  return os;
}

template <class Value, class T>
NonLoggable<T> Parse(const Value& v, formats::parse::To<NonLoggable<T>>) {
  return NonLoggable<T>{v.template As<T>()};
}

}  // namespace utils
