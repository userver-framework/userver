#pragma once

#include <regex>
#include <string_view>
#include <userver/utils/text.hpp>
#include "../utils/errors.hpp"
#include "../utils/make_error.hpp"


namespace real_medium::validators {

const std::string CheckLength(const std::string& value, std::string_view fieldName,
                            int minLen, int maxLen);

const std::string CheckLength(const userver::formats::json::Value& json,
                            const std::string_view fieldName, int minLen, int maxLen);

}  // namespace real_meidum::validators
