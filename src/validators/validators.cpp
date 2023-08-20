#include "validators.hpp"
#include "user_validators.hpp"
#include "utils/errors.hpp"

namespace real_medium::validator {

void validate(const dto::UserLoginDTO& dto) {
  if (!dto.email) {
    throw utils::error::ValidationException("email", "Field is missing");
  } else if (!ValidateEmail(dto.email.value())) {
    throw utils::error::ValidationException("email", "Invalid field");
  }

  if (!dto.password) {
    throw utils::error::ValidationException("password", "Field is missing");
  } else if (!ValidatePassword(dto.password.value())) {
    throw utils::error::ValidationException("password",
                                            "Invalid field");
  }
}

void validate(const dto::UserRegistrationDTO& dto) {
  if (!dto.username) {
    throw utils::error::ValidationException("username", "Field is missing");
  } else if (!ValidateEmail(dto.username.value())) {
    throw utils::error::ValidationException("username", "Invalid field");
  }

  if (!dto.email) {
    throw utils::error::ValidationException("email", "Field is missing");
  } else if (!ValidateEmail(dto.email.value())) {
    throw utils::error::ValidationException("email", "Invalid field");
  }

  if (!dto.password) {
    throw utils::error::ValidationException("password", "Field is missing");
  } else if (!ValidatePassword(dto.password.value())) {
    throw utils::error::ValidationException("password",
                                            "Invalid value");
  }
}

}  // namespace real_medium::validator