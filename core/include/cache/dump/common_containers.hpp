#pragma once

#include <map>
#include <optional>
#include <set>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <utils/meta.hpp>

#include <cache/dump/common.hpp>
#include <cache/dump/meta.hpp>
#include <cache/dump/meta_containers.hpp>
#include <cache/dump/operations.hpp>

/// @file cache/dump/common_containers.hpp
/// @brief Cache dump support for standard containers and optional
///
/// @note There are no traits in `CachingComponentBase`. If `T`
/// is writable/readable, we have to generate the code for cache dumps
/// regardless of `IsDumpEnabled`. So it's important that all Read-Write
/// operations for containers are SFINAE-correct.

namespace cache::dump {

/// @{
/// Container support
template <typename T>
std::enable_if_t<kIsContainer<T> && kIsWritable<meta::RangeValueType<T>>> Write(
    Writer& writer, const T& value) {
  writer.Write(std::size(value));
  for (const auto& item : value) {
    // explicit cast for vector<bool> shenanigans
    writer.Write(static_cast<const meta::RangeValueType<T>&>(item));
  }
}

template <typename T>
std::enable_if_t<kIsContainer<T> && kIsReadable<meta::RangeValueType<T>>, T>
Read(Reader& reader, To<T>) {
  const auto size = reader.Read<std::size_t>();
  T result{};
  if constexpr (kIsReservable<T>) {
    result.reserve(size);
  }
  for (std::size_t i = 0; i < size; ++i) {
    cache::dump::Insert(result, reader.Read<meta::RangeValueType<T>>());
  }
  return result;
}
/// @}

/// @{
/// Pair support (for maps)
template <typename T, typename U>
std::enable_if_t<kIsWritable<T> && kIsWritable<U>, void> Write(
    Writer& writer, const std::pair<T, U>& value) {
  writer.Write(value.first);
  writer.Write(value.second);
}

template <typename T, typename U>
std::enable_if_t<kIsReadable<T> && kIsReadable<U>, std::pair<T, U>> Read(
    Reader& reader, To<std::pair<T, U>>) {
  return {reader.Read<T>(), reader.Read<U>()};
}
/// @}

/// @{
/// std::optional support
template <typename T>
std::enable_if_t<kIsWritable<T>> Write(Writer& writer,
                                       const std::optional<T>& value) {
  writer.Write(value.has_value());
  if (value) writer.Write(*value);
}

template <typename T>
std::enable_if_t<kIsReadable<T>, std::optional<T>> Read(Reader& reader,
                                                        To<std::optional<T>>) {
  if (!reader.Read<bool>()) return std::nullopt;
  return reader.Read<T>();
}
/// @}

/// Allows reading `const T`, which is usually encountered as a member of some
/// container
template <typename T>
std::enable_if_t<kIsReadable<T>, T> Read(Reader& reader, To<const T>) {
  return Read(reader, To<T>{});
}

}  // namespace cache::dump
