#include "user_validators.hpp"

namespace real_medium::validator {

bool ValidateEmail(const std::string& email) {
  const std::regex pattern{R"((\w+)(\.|_)?(\w*)@(\w+)(\.(\w+))+)"};
  return regex_match(email.begin(), email.end(), pattern);
}
bool ValidatePassword(const std::string& password) { return !password.empty(); }
bool ValidateUsername(const std::string& username) { return !username.empty(); }

}  // namespace real_medium::validator