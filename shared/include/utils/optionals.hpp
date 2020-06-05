#pragma once

/// @file utils/optionals.hpp
/// @brief Helper functions for interoperation of boost and std optionals

#include <optional>
#include <string>

#include <fmt/format.h>
#include <boost/optional.hpp>

namespace utils {

template <class T>
std::optional<T> ToStdOptional(const boost::optional<T>& from) {
  if (!from) return {};
  return *from;
}

template <class T>
std::optional<T> ToStdOptional(boost::optional<T>&& from) {
  if (!from) return {};
  return std::move(*from);
}

template <class T>
std::optional<T> ToStdOptional(std::optional<T>&& from) {
  return std::move(from);
}

template <class T>
std::optional<T> ToStdOptional(const std::optional<T>& from) {
  return from;
}

template <class T>
std::string ToString(const boost::optional<T>& from) {
  return from ? fmt::format(" {}", *from) : "--";
}

template <class T>
std::string ToString(const std::optional<T>& from) {
  return from ? fmt::format(" {}", *from) : "--";
}

template <class T>
boost::optional<T> ToBoostOptional(const std::optional<T>& from) {
  if (!from) return {};
  return *from;
}

template <class T>
boost::optional<T> ToBoostOptional(std::optional<T>&& from) {
  if (!from) return {};
  return std::move(*from);
}

template <class T>
T* OptionalRefToPointer(boost::optional<T&> from) {
  if (!from) return nullptr;
  return std::addressof(*from);
}

}  // namespace utils
