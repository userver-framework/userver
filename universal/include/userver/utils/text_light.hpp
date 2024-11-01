#pragma once

/// @file userver/utils/text_light.hpp
/// @brief Text utilities that do not use locales
/// @ingroup userver_universal

#include <string>
#include <string_view>
#include <vector>

USERVER_NAMESPACE_BEGIN

/// @brief Text utilities
namespace utils::text {

/// Return trimmed copy of string.
std::string Trim(const std::string& str);

/// Trim string in-place.
std::string Trim(std::string&& str);

enum class SplitFlags {
    kNone = 0,
    kCompressAdjacentSeparators = 1 << 0,
};

/// Split string by separators
///
/// @snippet utils/text_light_test.cpp  SplitMultiple
std::vector<std::string> Split(
    std::string_view str,
    std::string_view separators,
    SplitFlags split_flags = SplitFlags::kCompressAdjacentSeparators
);

/// Split string by separators and return a non-owning container of chunks.
///
/// @warning Initial `str` should outlive the result of the function
///
/// @snippet utils/text_light_test.cpp  SplitStringViewMultiple
std::vector<std::string_view> SplitIntoStringViewVector(std::string_view str, std::string_view separators);

/// Join string
std::string Join(const std::vector<std::string>& strs, std::string_view sep);

/// Return number formatted
std::string Format(double value, int ndigits);

/// Return true if `hay` starts with `needle`, false otherwise.
constexpr bool StartsWith(std::string_view hay, std::string_view needle) noexcept {
    return hay.substr(0, needle.size()) == needle;
}

/// Return true if `hay` ends with `needle`, false otherwise.
constexpr bool EndsWith(std::string_view hay, std::string_view needle) noexcept {
    return hay.size() >= needle.size() && hay.substr(hay.size() - needle.size()) == needle;
}

/// Case insensitive (ASCII only) variant of StartsWith()
bool ICaseStartsWith(std::string_view hay, std::string_view needle) noexcept;

/// Case insensitive (ASCII only) variant of EndsWith()
bool ICaseEndsWith(std::string_view hay, std::string_view needle) noexcept;

/// Removes double quotes from front and back of string.
///
/// Examples:
/// @code
///    RemoveQuotes("\"test\"")      // returns "test"
///    RemoveQuotes("\"test")        // returns "\"test"
///    RemoveQuotes("'test'")        // returns "'test'"
///    RemoveQuotes("\"\"test\"\"")  // returns "\"test\""
/// @endcode
std::string RemoveQuotes(std::string_view str);

/// Checks whether the character is an ASCII character
bool IsAscii(char ch) noexcept;

/// Checks whether the character is a whitespace character in C locale
bool IsAsciiSpace(char ch) noexcept;

/// Checks if text contains only ASCII characters
bool IsAscii(std::string_view text) noexcept;

/// @brief UTF8 text utilities
namespace utf8 {

/// Returns the length in bytes of the UTF-8 code point by the first byte.
unsigned CodePointLengthByFirstByte(unsigned char c) noexcept;

/// `bytes` must not be a nullptr, `length` must not be 0.
bool IsWellFormedCodePoint(const unsigned char* bytes, std::size_t length) noexcept;

/// `bytes` must not be a nullptr, `length` must not be 0.
bool IsValid(const unsigned char* bytes, std::size_t length) noexcept;

/// returns number of utf-8 code points, text must be in utf-8 encoding
/// @throws std::runtime_error if not a valid UTF8 text
std::size_t GetCodePointsCount(std::string_view text);

/// Removes the longest (possible empty) suffix of `str` which is a proper
/// prefix of some utf-8 multibyte character. If `str` is not in utf-8 it may
/// remove some suffix of length up to 3.
void TrimTruncatedEnding(std::string& str);

/// @see void TrimTruncatedEnding(std::string& str)
/// @warning this **does not** change the original string
void TrimViewTruncatedEnding(std::string_view& view);

/// Returns position in `text` where utf-8 code point with position `pos` starts
/// OR `text.length()` if `text` contains less than or equal to `pos` points
/// @warning this **does not** check if `text` is valid utf-8 text
std::size_t GetTextPosByCodePointPos(std::string_view text, std::size_t pos) noexcept;

/// Removes the first `count` utf-8 code points from `text`
/// @warning this **does not** check if `text` is valid utf-8 text
void RemovePrefix(std::string& text, std::size_t count) noexcept;

/// @see void RemovePrefix(std::string& text, std::size_t count)
/// @warning this **does not** change the original string
void RemoveViewPrefix(std::string_view& text, std::size_t count) noexcept;

/// Takes the first `count` utf-8 code points from `text`
/// @warning this **does not** check if `text` is valid utf-8 text
void TakePrefix(std::string& text, std::size_t count) noexcept;

/// @see void TakePrefix(std::string& text, std::size_t count)
/// @warning this **does not** change the original string
void TakeViewPrefix(std::string_view& text, std::size_t count) noexcept;

}  // namespace utf8

/// Checks if text is in utf-8 encoding
bool IsUtf8(std::string_view text) noexcept;

/// Checks text on matching to the following conditions:
/// 1. text is in utf-8 encoding
/// 2. text does not contain any of control ascii characters
/// 3. if flag ascii is true than text contains only ascii characters
bool IsPrintable(std::string_view text, bool ascii_only = true) noexcept;

/// Checks if there are no embedded null ('\0') characters in text
bool IsCString(std::string_view text) noexcept;

/// convert CamelCase to snake_case(underscore)
std::string CamelCaseToSnake(std::string_view camel);

}  // namespace utils::text

USERVER_NAMESPACE_END
