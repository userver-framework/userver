#pragma once

/// @file http/url.hpp
/// @brief URL manipulation functions

#include <string>
#include <unordered_map>

#include <utils/string_view.hpp>

namespace http {

std::string UrlDecode(utils::string_view range);

std::string UrlEncode(utils::string_view input_string);

using Args = std::unordered_map<std::string, std::string>;

std::string MakeQuery(const Args& query_args);

std::string MakeQuery(
    std::initializer_list<std::pair<utils::string_view, utils::string_view>>
        query_args);

std::string MakeUrl(utils::string_view path, const Args& query_args);

std::string MakeUrl(
    utils::string_view path,
    std::initializer_list<std::pair<utils::string_view, utils::string_view>>
        query_args);

std::string ExtractMetaTypeFromUrl(const std::string& url);

}  // namespace http
