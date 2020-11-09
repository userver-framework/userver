#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace server::http::parser {

void ParseArgs(
    std::string_view args,
    std::unordered_map<std::string, std::vector<std::string>>& result);

std::string UrlDecode(std::string_view url);

}  // namespace server::http::parser
