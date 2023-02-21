#pragma once

/// @file userver/http/url.hpp
/// @brief URL manipulation functions

#include <map>
#include <string>
#include <string_view>
#include <unordered_map>

#include <userver/utils/str_icase.hpp>

USERVER_NAMESPACE_BEGIN

namespace http {

/// @brief Decode URL
[[deprecated("Use a more strict http::parser::UrlDecode instead")]] std::string
UrlDecode(std::string_view range);

/// @brief Encode as URL
std::string UrlEncode(std::string_view input_string);

using Args = std::unordered_map<std::string, std::string, utils::StrCaseHash>;
using MultiArgs = std::multimap<std::string, std::string>;

/// @brief Make an URL query
std::string MakeQuery(const Args& query_args);

/// @brief Make an URL query
std::string MakeQuery(
    const std::unordered_map<std::string, std::string>& query_args);

/// @brief Make an URL query
std::string MakeQuery(
    std::initializer_list<std::pair<std::string_view, std::string_view>>
        query_args);

/// @brief Make an URL with query arguments
std::string MakeUrl(std::string_view path, const Args& query_args);

/// @brief Make an URL with query arguments
std::string MakeUrl(
    std::string_view path,
    const std::unordered_map<std::string, std::string>& query_args);

/// @brief Make an URL with query arguments
std::string MakeUrl(std::string_view path, const Args& query_args,
                    MultiArgs query_multiargs);

/// @brief Make an URL with query arguments
std::string MakeUrl(
    std::string_view path,
    std::initializer_list<std::pair<std::string_view, std::string_view>>
        query_args);

/// @brief Returns URL part before the first '?' character
std::string ExtractMetaTypeFromUrl(const std::string& url);

/// @brief Returns HTTP path part of a URL
std::string ExtractPath(std::string_view url);

/// @brief Returns hostname part of a URL
std::string ExtractHostname(std::string_view url);

}  // namespace http

USERVER_NAMESPACE_END
