#pragma once

/// @file http/url.hpp
/// @brief URL manipulation functions

#include <string>
#include <string_view>
#include <unordered_map>

namespace http {

std::string UrlDecode(std::string_view range);

std::string UrlEncode(std::string_view input_string);

using Args = std::unordered_map<std::string, std::string>;

std::string MakeQuery(const Args& query_args);

std::string MakeQuery(
    std::initializer_list<std::pair<std::string_view, std::string_view>>
        query_args);

std::string MakeUrl(std::string_view path, const Args& query_args);

std::string MakeUrl(
    std::string_view path,
    std::initializer_list<std::pair<std::string_view, std::string_view>>
        query_args);

std::string ExtractMetaTypeFromUrl(const std::string& url);

}  // namespace http
