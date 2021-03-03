#pragma once

#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

#include <boost/numeric/conversion/cast.hpp>

#include <cache/dump/operations.hpp>
#include <cache/dump/unsafe.hpp>
#include <utils/meta.hpp>

namespace cache::dump {

/// Reads the rest of the data from `reader`
std::string ReadEntire(Reader& reader);

namespace impl {

template <typename T>
void WriteRaw(Writer& writer, T value) {
  static_assert(std::is_trivially_copyable_v<T>);
  WriteStringViewUnsafe(
      writer,
      std::string_view{reinterpret_cast<const char*>(&value), sizeof(value)});
}

template <typename T>
T ReadRaw(Reader& reader, To<T>) {
  static_assert(std::is_trivially_copyable_v<T>);
  T value;
  ReadStringViewUnsafe(reader, sizeof(T))
      .copy(reinterpret_cast<char*>(&value), sizeof(T));
  return value;
}

void WriteInteger(Writer& writer, std::uint64_t value);

std::uint64_t ReadInteger(Reader& reader);

template <typename Duration>
inline constexpr bool kIsDumpedAsNanoseconds =
    std::is_integral_v<typename Duration::rep> &&
    (Duration::period::num == 1) &&
    (Duration{1} <= std::chrono::milliseconds{1}) &&
    (1'000'000'000 % Duration::period::den == 0);

}  // namespace impl

/// @brief Write-only `std::string_view` support
/// @see `ReadStringViewUnsafe`
void Write(Writer& writer, std::string_view value);

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
  using std::chrono::duration, std::chrono::nanoseconds;

  // Durations, which on some systems represent
  // `std::chrono::*_clock::duration`, are serialized as `std::nanoseconds`
  // to avoid system dependency
  if constexpr (impl::kIsDumpedAsNanoseconds<duration<Rep, Period>>) {
    const auto count = std::chrono::duration_cast<nanoseconds>(value).count();

    if (nanoseconds{count} != value) {
      throw std::logic_error(
          "Trying to serialize a huge duration, it does not fit into "
          "std::chrono::nanoseconds type");
    }
    impl::WriteRaw(writer, count);
  } else {
    impl::WriteRaw(writer, value.count());
  }
}

template <typename Rep, typename Period>
std::chrono::duration<Rep, Period> Read(
    Reader& reader, To<std::chrono::duration<Rep, Period>>) {
  using std::chrono::duration, std::chrono::nanoseconds;

  if constexpr (impl::kIsDumpedAsNanoseconds<duration<Rep, Period>>) {
    const auto count = impl::ReadRaw(reader, To<nanoseconds::rep>{});
    return std::chrono::duration_cast<duration<Rep, Period>>(
        nanoseconds{count});
  } else {
    const auto count = impl::ReadRaw(reader, To<Rep>{});
    return duration<Rep, Period>{count};
  }
}
/// @}

/// @{
/// @brief `std::chrono::time_point` support
/// @note Only `system_clock` is supported, because `steady_clock` can only
/// be used within a single execution
template <typename Duration>
void Write(Writer& writer,
           std::chrono::time_point<std::chrono::system_clock, Duration> value) {
  writer.Write(value.time_since_epoch());
}

template <typename Duration>
auto Read(Reader& reader,
          To<std::chrono::time_point<std::chrono::system_clock, Duration>>) {
  return std::chrono::time_point<std::chrono::system_clock, Duration>{
      reader.Read<Duration>()};
}
/// @}

}  // namespace cache::dump
