#pragma once

/// @file userver/utils/text.hpp
/// @brief Text utilities

#include <locale>
#include <string>
#include <string_view>
#include <vector>

#include <boost/multiprecision/cpp_dec_float.hpp>

/// @brief Text utilities
namespace utils::text {

/// Return trimmed copy of string.
std::string Trim(const std::string& str);

/// Trim string in-place.
std::string Trim(std::string&& str);

/// Split string
std::vector<std::string> Split(std::string_view str, std::string_view sep);

/// Join string
std::string Join(const std::vector<std::string>& strs, std::string_view sep);

/// Return number formatted with specified locale
std::string Format(double value, const std::string& locale, int ndigits = 0,
                   bool is_fixed = true);

/// Return number formatted
std::string Format(double value, int ndigits);

/// Return cpp_dec_float_50 formatted
std::string Format(boost::multiprecision::cpp_dec_float_50 value, int ndigits);

bool StartsWith(std::string_view hay, std::string_view needle);

bool EndsWith(std::string_view hay, std::string_view needle);

/// Capitalizes the first letter of the str
std::string Capitalize(std::string_view str, const std::string& locale);

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

/// Returns a locale with the specified name
const std::locale& GetLocale(const std::string& name);

/// @brief UTF8 text utilities
namespace utf8 {

unsigned CodePointLengthByFirstByte(unsigned char c) noexcept;

/// `bytes` must not be a nullptr, `length` must not be 0.
bool IsWellFormedCodePoint(const unsigned char* bytes,
                           std::size_t length) noexcept;

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
/// @warning this DOES NOT change the original string
void TrimViewTruncatedEnding(std::string_view& view);

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
