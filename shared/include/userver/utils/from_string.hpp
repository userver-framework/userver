#pragma once

/// @file userver/utils/from_string.hpp
/// @brief @copybrief utils::FromString

#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeindex>
#include <typeinfo>

#include <userver/utils/meta.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

namespace impl {

[[noreturn]] void ThrowFromStringException(std::string_view message,
                                           std::string_view input,
                                           std::type_index resultType);

}  // namespace impl

/// @brief Extract the number contained in the string. No space characters or
/// other extra characters allowed. Supported types:
///
/// - Integer types. Leading plus or minus is allowed. The number is always
///   base-10.
/// - Floating-point types. The accepted number format is identical to
///   `std::strtod`.
///
/// \tparam T The type of the number to be parsed
/// \param str The string that contains the number
/// \return The extracted number
/// \throw std::runtime_error if the string does not contain an integer or
/// floating-point number in the specified format, or the string contains extra
/// junk, or the number does not fit into the provided type
template <typename T>
T FromString(const std::string& str) {
  static_assert(!std::is_const_v<T> && !std::is_volatile_v<T>);
  static_assert(!std::is_reference_v<T>);
  static_assert(meta::kIsInteger<T> || std::is_floating_point_v<T>);

  if (str.empty()) {
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
      return std::strtof(str.data(), &end);
    } else if constexpr (std::is_same_v<T, double>) {
      return std::strtod(str.data(), &end);
    } else if constexpr (std::is_same_v<T, long double>) {
      return std::strtold(str.data(), &end);
    } else if constexpr (std::is_signed_v<T>) {
      return std::strtoll(str.data(), &end, 10);
    } else if constexpr (std::is_unsigned_v<T>) {
      return std::strtoull(str.data(), &end, 10);
    }
  }();

  if (errno == ERANGE) {
    impl::ThrowFromStringException("overflow", str, typeid(T));
  }
  if (end == str.data()) {
    impl::ThrowFromStringException("no number found", str, typeid(T));
  }

  if constexpr (meta::kIsInteger<T>) {
    if (result < std::numeric_limits<T>::min() ||
        result > std::numeric_limits<T>::max()) {
      impl::ThrowFromStringException("overflow", str, typeid(T));
    }
    if (std::is_unsigned_v<T> && str[0] == '-' && result != 0) {
      impl::ThrowFromStringException("overflow", str, typeid(T));
    }
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

  return static_cast<T>(result);
}

}  // namespace utils

USERVER_NAMESPACE_END
