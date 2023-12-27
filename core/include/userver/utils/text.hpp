#pragma once

/// @file userver/utils/text.hpp
/// @brief Text utilities

#include <locale>

#include <userver/utils/text_light.hpp>

USERVER_NAMESPACE_BEGIN

/// @brief Text utilities
namespace utils::text {

inline const std::string kEnLocale{"en_US.UTF-8"};

/// Return number formatted with specified locale
std::string Format(double value, const std::string& locale, int ndigits = 0,
                   bool is_fixed = true);

/// Transform letters to lower case
std::string ToLower(std::string_view str,
                    const std::string& locale = kEnLocale);

/// Transform letters to upper case
std::string ToUpper(std::string_view str,
                    const std::string& locale = kEnLocale);

/// Capitalizes the first letter of the str
std::string Capitalize(std::string_view str, const std::string& locale);

/// Returns a locale with the specified name
const std::locale& GetLocale(const std::string& name);

}  // namespace utils::text

USERVER_NAMESPACE_END
