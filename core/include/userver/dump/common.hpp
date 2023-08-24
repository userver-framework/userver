#pragma once

/// @file userver/dump/common.hpp
/// @brief Serialization and deserialization of integral, floating point,
/// string, `std::chrono`, `enum` and `uuid` types for dumps.
///
/// @ingroup userver_dump_read_write

#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>

#include <boost/numeric/conversion/cast.hpp>

#include <userver/dump/operations.hpp>
#include <userver/dump/unsafe.hpp>
#include <userver/utils/meta_light.hpp>

namespace boost::uuids {
struct uuid;
}

USERVER_NAMESPACE_BEGIN

namespace decimal64 {
template <int Prec, typename RoundPolicy>
class Decimal;
}  // namespace decimal64

namespace formats::json {
class Value;
}  // namespace formats::json

namespace dump {

/// @brief Reads the rest of the data from `reader`
std::string ReadEntire(Reader& reader);

namespace impl {

/// @brief Helpers for serialization of trivially-copyable types
template <typename T>
void WriteTrivial(Writer& writer, T value) {
  static_assert(std::is_trivially_copyable_v<T>);
  // TODO: endianness
  WriteStringViewUnsafe(
      writer,
      std::string_view{reinterpret_cast<const char*>(&value), sizeof(value)});
}

/// @brief Helpers for deserialization trivially-copyable types
template <typename T>
T ReadTrivial(Reader& reader) {
  static_assert(std::is_trivially_copyable_v<T>);
  T value{};
  // TODO: endianness
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

/// @brief `std::string` serialization support
void Write(Writer& writer, const std::string& value);

/// @brief `std::string` deserialization support
std::string Read(Reader& reader, To<std::string>);

/// @brief Allows writing string literals
void Write(Writer& writer, const char* value);

/// @brief Integral types serialization support
template <typename T>
std::enable_if_t<meta::kIsInteger<T>> Write(Writer& writer, T value) {
  if constexpr (sizeof(T) == 1) {
    impl::WriteTrivial(writer, value);
  } else {
    impl::WriteInteger(writer, static_cast<std::uint64_t>(value));
  }
}

/// @brief Integral types deserialization support
template <typename T>
std::enable_if_t<meta::kIsInteger<T>, T> Read(Reader& reader, To<T>) {
  if constexpr (sizeof(T) == 1) {
    return impl::ReadTrivial<T>(reader);
  }

  const auto raw = impl::ReadInteger(reader);

  if constexpr (std::is_signed_v<T>) {
    return boost::numeric_cast<T>(static_cast<std::int64_t>(raw));
  } else {
    return boost::numeric_cast<T>(raw);
  }
}

/// @brief Floating-point serialization support
template <typename T>
std::enable_if_t<std::is_floating_point_v<T>> Write(Writer& writer, T value) {
  impl::WriteTrivial(writer, value);
}

/// @brief Floating-point deserialization support
template <typename T>
std::enable_if_t<std::is_floating_point_v<T>, T> Read(Reader& reader, To<T>) {
  return impl::ReadTrivial<T>(reader);
}

/// @brief bool serialization support
void Write(Writer& writer, bool value);

/// @brief bool deserialization support
bool Read(Reader& reader, To<bool>);

/// @brief enum serialization support
template <typename T>
std::enable_if_t<std::is_enum_v<T>> Write(Writer& writer, T value) {
  writer.Write(static_cast<std::underlying_type_t<T>>(value));
}

/// @brief enum deserialization support
template <typename T>
std::enable_if_t<std::is_enum_v<T>, T> Read(Reader& reader, To<T>) {
  return static_cast<T>(reader.Read<std::underlying_type_t<T>>());
}

/// @brief `std::chrono::duration` serialization support
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
    impl::WriteTrivial(writer, count);
  } else {
    impl::WriteTrivial(writer, value.count());
  }
}

/// @brief `std::chrono::duration` deserialization support
template <typename Rep, typename Period>
std::chrono::duration<Rep, Period> Read(
    Reader& reader, To<std::chrono::duration<Rep, Period>>) {
  using std::chrono::duration, std::chrono::nanoseconds;

  if constexpr (impl::kIsDumpedAsNanoseconds<duration<Rep, Period>>) {
    const auto count = impl::ReadTrivial<nanoseconds::rep>(reader);
    return std::chrono::duration_cast<duration<Rep, Period>>(
        nanoseconds{count});
  } else {
    const auto count = impl::ReadTrivial<Rep>(reader);
    return duration<Rep, Period>{count};
  }
}

/// @brief `std::chrono::time_point` serialization support
/// @note Only `system_clock` is supported, because `steady_clock` can only
/// be used within a single execution
template <typename Duration>
void Write(Writer& writer,
           std::chrono::time_point<std::chrono::system_clock, Duration> value) {
  writer.Write(value.time_since_epoch());
}

/// @brief `std::chrono::time_point` deserialization support
/// @note Only `system_clock` is supported, because `steady_clock` can only
/// be used within a single execution
template <typename Duration>
auto Read(Reader& reader,
          To<std::chrono::time_point<std::chrono::system_clock, Duration>>) {
  return std::chrono::time_point<std::chrono::system_clock, Duration>{
      reader.Read<Duration>()};
}

/// @brief `boost::uuids::uuid` serialization support
void Write(Writer& writer, const boost::uuids::uuid& value);

/// @brief `boost::uuids::uuid` deserialization support
boost::uuids::uuid Read(Reader& reader, To<boost::uuids::uuid>);

/// @brief decimal64::Decimal serialization support
template <int Prec, typename RoundPolicy>
inline void Write(Writer& writer,
                  const decimal64::Decimal<Prec, RoundPolicy>& dec) {
  writer.Write(dec.AsUnbiased());
}

/// @brief decimal64::Decimal deserialization support
template <int Prec, typename RoundPolicy>
auto Read(Reader& reader, dump::To<decimal64::Decimal<Prec, RoundPolicy>>) {
  return decimal64::Decimal<Prec, RoundPolicy>::FromUnbiased(
      reader.Read<int64_t>());
}

/// @brief formats::json::Value serialization support
void Write(Writer& writer, const formats::json::Value& value);

/// @brief formats::json::Value deserialization support
formats::json::Value Read(Reader& reader, To<formats::json::Value>);

}  // namespace dump

USERVER_NAMESPACE_END
