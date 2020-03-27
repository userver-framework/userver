#pragma once

#include <functional>
#include <iosfwd>

#include <boost/functional/hash_fwd.hpp>

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

template <class T>
bool operator==(const NonLoggable<T>& lhs, const NonLoggable<T>& rhs) {
  return lhs.GetUnprotectedRawValue() == rhs.GetUnprotectedRawValue();
}

template <class T>
bool operator!=(const NonLoggable<T>& lhs, const NonLoggable<T>& rhs) {
  return !(lhs.GetUnprotectedRawValue() == rhs.GetUnprotectedRawValue());
}

template <class T>
bool operator<(const NonLoggable<T>& lhs, const NonLoggable<T>& rhs) {
  return lhs.GetUnprotectedRawValue() < rhs.GetUnprotectedRawValue();
}

template <class T>
bool operator>(const NonLoggable<T>& lhs, const NonLoggable<T>& rhs) {
  return rhs.GetUnprotectedRawValue() < lhs.GetUnprotectedRawValue();
}

template <class T>
bool operator<=(const NonLoggable<T>& lhs, const NonLoggable<T>& rhs) {
  return !(rhs.GetUnprotectedRawValue() < lhs.GetUnprotectedRawValue());
}

template <class T>
bool operator>=(const NonLoggable<T>& lhs, const NonLoggable<T>& rhs) {
  return !(lhs.GetUnprotectedRawValue() < rhs.GetUnprotectedRawValue());
}

template <class T>
std::size_t hash_value(const NonLoggable<T>& value) {
  return boost::hash<T>{}(value.GetUnprotectedRawValue());
}

}  // namespace utils

namespace std {

template <class T>
struct hash<utils::NonLoggable<T>> : hash<T> {
  std::size_t operator()(const utils::NonLoggable<T>& value) const noexcept(
      noexcept(std::declval<const hash<T>>()(std::declval<const T&>()))) {
    return hash<T>::operator()(value.GetUnprotectedRawValue());
  }
};

}  // namespace std
