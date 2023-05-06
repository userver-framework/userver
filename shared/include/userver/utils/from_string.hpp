#pragma once

/// @file userver/utils/from_string.hpp
/// @brief @copybrief utils::FromString

#include <cctype>
#include <cerrno>
#include <charconv>
#include <cstdlib>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeindex>
#include <typeinfo>

#include <userver/utils/meta_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace impl {

[[noreturn]] void ThrowFromStringException(std::string_view message,
                                           std::string_view input,
                                           std::type_index resultType);

template <typename T>
std::enable_if_t<std::is_floating_point_v<T>, T> FromString(const char* str) {
  static_assert(!std::is_const_v<T> && !std::is_volatile_v<T>);
  static_assert(!std::is_reference_v<T>);

  if (str == nullptr) {
    impl::ThrowFromStringException("nullptr string", "<null>", typeid(T));
  }
  if (str[0] == '\0') {
    impl::ThrowFromStringException("empty string", str, typeid(T));
  }
  if (std::isspace(str[0])) {
    impl::ThrowFromStringException("leading spaces are not allowed", str,
                                   typeid(T));
  }

  char* end = nullptr;
  errno = 0;

  const auto result = [&] {
    if constexpr (std::is_same_v<T, float>) {
      return std::strtof(str, &end);
    } else if constexpr (std::is_same_v<T, double>) {
      return std::strtod(str, &end);
    } else if constexpr (std::is_same_v<T, long double>) {
      return std::strtold(str, &end);
    }
  }();

  if (errno == ERANGE) {
    impl::ThrowFromStringException("overflow", str, typeid(T));
  }
  if (end == str) {
    impl::ThrowFromStringException("no number found", str, typeid(T));
  }

  if (*end != '\0') {
    if (std::isspace(*end)) {
      impl::ThrowFromStringException("trailing spaces are not allowed", str,
                                     typeid(T));
    } else {
      impl::ThrowFromStringException(
          "extra junk at the end of the string is not allowed", str, typeid(T));
    }
  }

  return result;
}

template <typename T>
std::enable_if_t<std::is_floating_point_v<T>, T> FromString(
    const std::string& str) {
  return FromString<T>(str.data());
}

template <typename T>
std::enable_if_t<std::is_floating_point_v<T>, T> FromString(
    std::string_view str) {
  static constexpr std::size_t kSmallBufferSize = 32;

  if (str.size() >= kSmallBufferSize) {
    return FromString<T>(std::string{str});
  }

  char buffer[kSmallBufferSize];
  std::copy(str.data(), str.data() + str.size(), buffer);
  buffer[str.size()] = '\0';

  return FromString<T>(buffer);
}

template <typename T>
std::enable_if_t<meta::kIsInteger<T>, T> FromString(std::string_view str) {
  static_assert(!std::is_const_v<T> && !std::is_volatile_v<T>);
  static_assert(!std::is_reference_v<T>);

  if (str.empty()) {
    impl::ThrowFromStringException("empty string", str, typeid(T));
  }
  if (std::isspace(str[0])) {
    impl::ThrowFromStringException("leading spaces are not allowed", str,
                                   typeid(T));
  }

  std::size_t offset = 0;

  // to allow leading plus
  if (str.size() > 1 && str[0] == '+' && str[1] == '-') {
    impl::ThrowFromStringException("no number found", str, typeid(T));
  }
  if (str[0] == '+') offset = 1;

  // to process '-0' correctly
  if (std::is_unsigned_v<T> && str[0] == '-') offset = 1;

  T result{};
  const auto [end, error_code] =
      std::from_chars(str.data() + offset, str.data() + str.size(), result);

  if (error_code == std::errc::result_out_of_range) {
    impl::ThrowFromStringException("overflow", str, typeid(T));
  }
  if (error_code == std::errc::invalid_argument) {
    impl::ThrowFromStringException("no number found", str, typeid(T));
  }

  if (std::is_unsigned_v<T> && str[0] == '-' && result != 0) {
    impl::ThrowFromStringException("overflow", str, typeid(T));
  }

  if (end != str.data() + str.size()) {
    if (std::isspace(*end)) {
      impl::ThrowFromStringException("trailing spaces are not allowed", str,
                                     typeid(T));
    } else {
      impl::ThrowFromStringException(
          "extra junk at the end of the string is not allowed", str, typeid(T));
    }
  }

  return result;
}

}  // namespace impl

/// @brief Extract the number contained in the string. No space characters or
/// other extra characters allowed. Supported types:
///
/// - Integer types. Leading plus or minus is allowed. The number is always
///   base-10.
/// - Floating-point types. The accepted number format is identical to
///   `std::strtod`.
///
/// @tparam T The type of the number to be parsed
/// @param str The string that contains the number
/// @return The extracted number
/// @throw std::runtime_error if the string does not contain an integer or
/// floating-point number in the specified format, or the string contains extra
/// junk, or the number does not fit into the provided type
template <typename T, typename StringType,
          typename = std::enable_if_t<
              std::is_convertible_v<StringType, std::string_view>>>
T FromString(const StringType& str) {
  return impl::FromString<T>(str);
}

}  // namespace utils

USERVER_NAMESPACE_END
