#pragma once

#include <regex>
#include <string_view>

namespace real_medium::validator {

bool ValidateEmail(const std::string& email);
bool ValidatePassword(const std::string& password);

}  // namespace real_medium::validator