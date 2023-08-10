#pragma once

#include <regex>
#include <string_view>

namespace real_medium::validators {

bool ValidateEmail(std::string_view email);
bool ValidatePassword(std::string_view password);

}  // namespace real_medium::validators