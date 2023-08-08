#include "user_validators.hpp"

namespace real_medium::validators {

bool ValidateEmail(std::string_view email) {
  const std::regex pattern("(\\w+)(\\.|_)?(\\w*)@(\\w+)(\\.(\\w+))+");
  return regex_match(email.begin(), email.end(), pattern);
}
bool ValidatePassword(std::string_view password) { return !password.empty(); }

}  // namespace real_medium::validators