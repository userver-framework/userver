#pragma once

/// @file userver/http/parser/http_request_parse_args.hpp
/// @brief Strict URL arguments parsing

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include <userver/utils/str_icase.hpp>

USERVER_NAMESPACE_BEGIN

namespace http::parser {

/// Strict URL decoder that throws std::runtime_error on bad input
std::string UrlDecode(std::string_view url);

void ParseArgs(std::string_view args,
               std::unordered_map<std::string, std::vector<std::string>,
                                  utils::StrCaseHash>& result);

using ArgsConsumer =
    std::function<void(std::string&& key, std::string&& value)>;

void ParseAndConsumeArgs(std::string_view args, ArgsConsumer handler);

}  // namespace http::parser

USERVER_NAMESPACE_END
