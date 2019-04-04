#pragma once

#include <string>
#include <unordered_map>

#include <utils/string_view.hpp>

namespace http {

std::string UrlDecode(utils::string_view encoded);

std::string UrlEncode(utils::string_view decoded);

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
