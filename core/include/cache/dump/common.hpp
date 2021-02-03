#pragma once

#include <chrono>
#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>

#include <boost/numeric/conversion/cast.hpp>

#include <utils/meta.hpp>

#include <cache/dump/operations.hpp>

namespace cache::dump {

namespace impl {

template <typename T>
void WriteRaw(Writer& writer, T value) {
  static_assert(std::is_trivially_copyable_v<T>);
  writer.WriteRaw(
      std::string_view{reinterpret_cast<const char*>(&value), sizeof(value)});
}

template <typename T>
T ReadRaw(Reader& reader, To<T>) {
  static_assert(std::is_trivially_copyable_v<T>);
  T value;
  reader.ReadRaw(sizeof(T)).copy(reinterpret_cast<char*>(&value), sizeof(T));
  return value;
}

void WriteInteger(Writer& writer, std::uint64_t value);

std::uint64_t ReadInteger(Reader& reader);

}  // namespace impl

/// @brief Write-only `std::string_view` support
/// @see `ReadStringViewUnsafe`
void Write(Writer& writer, std::string_view value);

/// @brief Reads a `std::string_view`
/// @warning The `string_view` will be invalidated on the next `Read` operation
std::string_view ReadStringViewUnsafe(Reader& reader);

// TODO TAXICOMMON-3483 remove
std::string_view Read(Reader& reader, To<std::string_view>);

/// @{
/// `std::string` support
void Write(Writer& writer, const std::string& value);

std::string Read(Reader& reader, To<std::string>);
/// @}

/// Allows writing string literals
void Write(Writer& writer, const char* value);

/// @{
/// Integer support
template <typename T>
std::enable_if_t<meta::kIsInteger<T>> Write(Writer& writer, T value) {
  if constexpr (sizeof(T) == 1) {
    impl::WriteRaw(writer, value);
  } else {
    impl::WriteInteger(writer, static_cast<std::uint64_t>(value));
  }
}

template <typename T>
std::enable_if_t<meta::kIsInteger<T>, T> Read(Reader& reader, To<T> to) {
  // NOLINTNEXTLINE(bugprone-suspicious-semicolon)
  if constexpr (sizeof(T) == 1) {
    return impl::ReadRaw(reader, to);
  }

  const auto raw = impl::ReadInteger(reader);

  if constexpr (std::is_signed_v<T>) {
    return boost::numeric_cast<T>(static_cast<std::int64_t>(raw));
  } else {
    return boost::numeric_cast<T>(raw);
  }
}
/// @}

/// @{
/// Floating-point support
template <typename T>
std::enable_if_t<std::is_floating_point_v<T>> Write(Writer& writer, T value) {
  impl::WriteRaw(writer, value);
}

template <typename T>
std::enable_if_t<std::is_floating_point_v<T>, T> Read(Reader& reader,
                                                      To<T> to) {
  return impl::ReadRaw(reader, to);
}
/// @}

/// @{
/// `bool` support
void Write(Writer& writer, bool value);

bool Read(Reader& reader, To<bool>);
/// @}

/// @{
/// `enum` support
template <typename T>
std::enable_if_t<std::is_enum_v<T>> Write(Writer& writer, T value) {
  writer.Write(static_cast<std::underlying_type_t<T>>(value));
}

template <typename T>
std::enable_if_t<std::is_enum_v<T>, T> Read(Reader& reader, To<T>) {
  return static_cast<T>(reader.Read<std::underlying_type_t<T>>());
}
/// @}

/// @{
/// `std::chrono::duration` support
template <typename Rep, typename Period>
void Write(Writer& writer, std::chrono::duration<Rep, Period> value) {
  writer.Write(value.count());
}

template <typename Rep, typename Period>
std::chrono::duration<Rep, Period> Read(
    Reader& reader, To<std::chrono::duration<Rep, Period>>) {
  return std::chrono::duration<Rep, Period>{reader.Read<Rep>()};
}
///

/// @{
/// @brief `std::chrono::time_point` support
/// @note Only `system_clock` is supported, because `steady_clock` can only
/// be used within a single execution
template <typename Duration>
void Write(Writer& writer,
           std::chrono::time_point<std::chrono::system_clock, Duration> value) {
  // don't use integer compression for time_point
  impl::WriteRaw(writer, value.time_since_epoch());
}

template <typename Duration>
auto Read(Reader& reader,
          To<std::chrono::time_point<std::chrono::system_clock, Duration>>) {
  return std::chrono::time_point<std::chrono::system_clock, Duration>{
      impl::ReadRaw(reader, To<Duration>{})};
}
/// @}

}  // namespace cache::dump
