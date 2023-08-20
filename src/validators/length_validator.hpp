#pragma once

#include <regex>
#include <string_view>
#include <userver/utils/text.hpp>
#include "../utils/errors.hpp"
#include "../utils/make_error.hpp"


namespace real_medium::validator {

void CheckLength(const std::string& value, std::string_view fieldName,
                            std::size_t minLen, std::size_t maxLen);

}  // namespace real_meidum::validators
