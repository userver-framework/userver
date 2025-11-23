#pragma once

/// @file userver/utils/from_string.hpp
/// @brief @copybrief utils::FromString
/// @ingroup userver_universal

#include <cctype>
#include <cerrno>
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeindex>
#include <typeinfo>

#include <userver/utils/zstring_view.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

class FromStringException : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

namespace impl {

template <typename T, typename = void>
struct IsFromCharsConvertible : std::false_type {};

template <typename T>
struct IsFromCharsConvertible<
    T,
    std::void_t<decltype(std::from_chars(std::declval<const char*>(), std::declval<const char*>(), std::declval<T&>())
    )>> : std::true_type {};

// libstdc++ before 13.1 parse long double incorrectly
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE < 13
template <>
struct IsFromCharsConvertible<long double> : std::false_type {};
#endif

template <class T>
inline constexpr bool kIsFromCharsConvertible = IsFromCharsConvertible<T>::value;

[[noreturn]] void ThrowFromStringException(
    std::string_view message,
    std::string_view input,
    std::type_index result_type
);

template <typename T>
std::enable_if_t<std::is_floating_point_v<T> && !kIsFromCharsConvertible<T>, T> FromString(utils::zstring_view str) {
    static_assert(!std::is_const_v<T> && !std::is_volatile_v<T>);
    static_assert(!std::is_reference_v<T>);

    if (str.empty()) {
        impl::ThrowFromStringException("empty string", str, typeid(T));
    }
    if (std::isspace(str.front())) {
        impl::ThrowFromStringException("leading spaces are not allowed", str, typeid(T));
    }
    if (str.size() > 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        impl::ThrowFromStringException("extra junk at the end of the string is not allowed", str, typeid(T));
    }

    errno = 0;
    char* end = nullptr;

    const auto result = [&] {
        if constexpr (std::is_same_v<T, float>) {
            return std::strtof(str.c_str(), &end);
        } else if constexpr (std::is_same_v<T, double>) {
            return std::strtod(str.c_str(), &end);
        } else if constexpr (std::is_same_v<T, long double>) {
            return std::strtold(str.c_str(), &end);
        }
    }();

    if (errno == ERANGE && !(result < 1 && result > 0.0)) {
        impl::ThrowFromStringException("overflow", str, typeid(T));
    }

    if (end == str.c_str()) {
        impl::ThrowFromStringException("no number found", str, typeid(T));
    }

    if (end != str.data() + str.size()) {
        if (std::isspace(*end)) {
            impl::ThrowFromStringException("trailing spaces are not allowed", str, typeid(T));
        } else {
            impl::ThrowFromStringException("extra junk at the end of the string is not allowed", str, typeid(T));
        }
    }

    return result;
}

template <typename T>
std::enable_if_t<std::is_floating_point_v<T> && !kIsFromCharsConvertible<T>, T> FromString(const std::string& str) {
    return impl::FromString<T>(utils::zstring_view{str});
}

template <typename T>
std::enable_if_t<std::is_floating_point_v<T> && !kIsFromCharsConvertible<T>, T> FromString(const char* str) {
    return impl::FromString<T>(utils::zstring_view{str});
}

template <typename T>
std::enable_if_t<std::is_floating_point_v<T> && !kIsFromCharsConvertible<T>, T> FromString(std::string_view str) {
    static constexpr std::size_t kSmallBufferSize = 32;

    if (str.size() >= kSmallBufferSize) {
        const std::string buffer{str};
        return impl::FromString<T>(utils::zstring_view{buffer});
    }

    char buffer[kSmallBufferSize];
    std::copy(str.data(), str.data() + str.size(), buffer);
    buffer[str.size()] = '\0';

    return impl::FromString<T>(utils::zstring_view::UnsafeMake(buffer, str.size()));
}

template <typename T>
std::enable_if_t<kIsFromCharsConvertible<T>, T> FromString(std::string_view str) {
    static_assert(!std::is_const_v<T> && !std::is_volatile_v<T>);
    static_assert(!std::is_reference_v<T>);

    if (str.empty()) {
        impl::ThrowFromStringException("empty string", str, typeid(T));
    }
    if (std::isspace(str[0])) {
        impl::ThrowFromStringException("leading spaces are not allowed", str, typeid(T));
    }

    std::size_t offset = 0;

    // to allow leading plus
    if (str.size() > 1 && str[0] == '+' && str[1] == '-') {
        impl::ThrowFromStringException("no number found", str, typeid(T));
    }
    if (str[0] == '+') {
        offset = 1;
    }

    // to process '-0' correctly
    if (std::is_unsigned_v<T> && str[0] == '-') {
        offset = 1;
    }

    T result{};
    const auto [end, error_code] = std::from_chars(str.data() + offset, str.data() + str.size(), result);

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
            impl::ThrowFromStringException("trailing spaces are not allowed", str, typeid(T));
        } else {
            impl::ThrowFromStringException("extra junk at the end of the string is not allowed", str, typeid(T));
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
template <
    typename T,
    typename StringType,
    typename = std::enable_if_t<std::is_convertible_v<StringType, std::string_view>>>
T FromString(const StringType& str) {
    return impl::FromString<T>(str);
}

std::int64_t FromHexString(std::string_view str);

}  // namespace utils

USERVER_NAMESPACE_END
