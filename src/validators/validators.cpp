#include "validators.hpp"
#include "user_validators.hpp"
#include "utils/errors.hpp"

namespace real_medium::validator {

void validate(const dto::UserLoginDTO& dto) {
  if (!ValidateEmail(dto.email)) {
    throw utils::error::ValidationException("email", "Invalid 'email' field");
  }

  if (!ValidatePassword(dto.password)) {
    throw utils::error::ValidationException("email", "Invalid 'password' field");
  }
}

}  // namespace real_medium::validator