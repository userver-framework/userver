#pragma once

/// @file userver/utils/from_string.hpp
/// @brief @copybrief utils::FromString
/// @ingroup userver_universal

#include <cctype>
#include <cerrno>
#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeinfo>

#include <userver/utils/expected.hpp>
#include <userver/utils/zstring_view.hpp>

USERVER_NAMESPACE_BEGIN

namespace utils {

/// @brief Conversion error code.
enum class FromStringErrorCode {
    /// @brief String contains leading whitespace characters.
    kLeadingSpaces = 1,

    /// @brief String contains invalid (non-digit) characters at the end.
    kTrailingJunk = 2,

    /// @brief String does not contain a number.
    kNoNumber = 3,

    /// @brief Conversion result is out of the valid range of the specified type.
    kOverflow = 4
};

/// @brief Converts @a code to string representation.
constexpr inline std::string_view ToString(FromStringErrorCode code) noexcept {
    switch (code) {
        case FromStringErrorCode::kLeadingSpaces:
            return "leading spaces are not allowed";
        case FromStringErrorCode::kTrailingJunk:
            return "extra junk at the end of the string is not allowed";
        case FromStringErrorCode::kNoNumber:
            return "no number found";
        case FromStringErrorCode::kOverflow:
            return "overflow";
        default:
            return "unknown";
    }
}

/// @brief Function `utils::FromString` exception type.
class FromStringException : public std::runtime_error {
public:
    /// @brief Creates exception for @a code .
    FromStringException(FromStringErrorCode code, const std::string& what);

    /// @brief Returns conversion error code.
    FromStringErrorCode GetCode() const noexcept { return code_; }

private:
    FromStringErrorCode code_;
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
    FromStringErrorCode code,
    std::string_view input,
    const std::type_info& result_type
);

template <typename T>
std::enable_if_t<std::is_floating_point_v<T> && !kIsFromCharsConvertible<T>, expected<T, FromStringErrorCode>>
FromString(utils::zstring_view str) noexcept {
    static_assert(!std::is_const_v<T> && !std::is_volatile_v<T>);
    static_assert(!std::is_reference_v<T>);

    if (str.empty()) {
        return unexpected{FromStringErrorCode::kNoNumber};
    }
    if (std::isspace(str.front())) {
        return unexpected{FromStringErrorCode::kLeadingSpaces};
    }
    if (str.size() > 2 && str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        return unexpected{FromStringErrorCode::kTrailingJunk};
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
        return unexpected{FromStringErrorCode::kOverflow};
    }

    if (end == str.c_str()) {
        return unexpected{FromStringErrorCode::kNoNumber};
    }

    if (end != str.data() + str.size()) {
        return unexpected{FromStringErrorCode::kTrailingJunk};
    }

    return result;
}

template <typename T>
std::enable_if_t<std::is_floating_point_v<T> && !kIsFromCharsConvertible<T>, expected<T, FromStringErrorCode>>
FromString(const std::string& str) noexcept {
    return impl::FromString<T>(utils::zstring_view{str});
}

template <typename T>
std::enable_if_t<std::is_floating_point_v<T> && !kIsFromCharsConvertible<T>, expected<T, FromStringErrorCode>>
FromString(const char* str) noexcept {
    return impl::FromString<T>(utils::zstring_view{str});
}

template <typename T>
std::enable_if_t<std::is_floating_point_v<T> && !kIsFromCharsConvertible<T>, expected<T, FromStringErrorCode>>
FromString(std::string_view str) noexcept {
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
std::enable_if_t<kIsFromCharsConvertible<T>, expected<T, FromStringErrorCode>> FromString(std::string_view str
) noexcept {
    static_assert(!std::is_const_v<T> && !std::is_volatile_v<T>);
    static_assert(!std::is_reference_v<T>);

    if (str.empty()) {
        return unexpected{FromStringErrorCode::kNoNumber};
    }
    if (std::isspace(str[0])) {
        return unexpected{FromStringErrorCode::kLeadingSpaces};
    }

    std::size_t offset = 0;

    // to allow leading plus
    if (str.size() > 1 && str[0] == '+' && str[1] == '-') {
        return unexpected{FromStringErrorCode::kNoNumber};
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
        return unexpected{FromStringErrorCode::kOverflow};
    }
    if (error_code == std::errc::invalid_argument) {
        return unexpected{FromStringErrorCode::kNoNumber};
    }

    if (std::is_unsigned_v<T> && str[0] == '-' && result != 0) {
        return unexpected{FromStringErrorCode::kOverflow};
    }

    if (end != str.data() + str.size()) {
        return unexpected{FromStringErrorCode::kTrailingJunk};
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
/// @throw FromStringException if the string does not contain an integer or
/// floating-point number in the specified format, or the string contains extra
/// junk, or the number does not fit into the provided type
template <
    typename T,
    typename StringType,
    typename = std::enable_if_t<std::is_convertible_v<StringType, std::string_view>>>
T FromString(const StringType& str) {
    const auto result = impl::FromString<T>(str);

    if (result) {
        return result.value();
    } else {
        impl::ThrowFromStringException(result.error(), str, typeid(T));
    }
}

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
/// @return `utils::expected` with the conversion result or error code
template <
    typename T,
    typename StringType,
    typename = std::enable_if_t<std::is_convertible_v<StringType, std::string_view>>>
expected<T, FromStringErrorCode> FromStringNoThrow(const StringType& str) noexcept {
    return impl::FromString<T>(str);
}

std::int64_t FromHexString(std::string_view str);

}  // namespace utils

USERVER_NAMESPACE_END
