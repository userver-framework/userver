#include "validators.hpp"
#include "article_validators.hpp"
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
    throw utils::error::ValidationException("password", "Invalid field");
  }
}

void validate(const dto::UserRegistrationDTO& dto) {
  if (!dto.username) {
    throw utils::error::ValidationException("username", "Field is missing");
  } else if (!ValidateUsername(dto.username.value())) {
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
    throw utils::error::ValidationException("password", "Invalid value");
  }
}
void validate(const dto::UserUpdateDTO& dto) {
  if (dto.username && !ValidateUsername(dto.username.value())) {
    throw utils::error::ValidationException("username", "Invalid field");
  }

  if (dto.email && !ValidateEmail(dto.email.value())) {
    throw utils::error::ValidationException("email", "Invalid field");
  }

  if (dto.password && !ValidatePassword(dto.password.value())) {
    throw utils::error::ValidationException("password", "Invalid field");
  }
}
void validate(const dto::AddCommentDTO& dto) {
  if (!dto.body) {
    throw utils::error::ValidationException("body", "Field is missing");
  }

  if (dto.body.value().empty()) {
    throw utils::error::ValidationException("body", "Invalid field");
  }
}

void validate(const dto::CreateArticleRequest& dto) {
  if (!dto.title) {
    throw utils::error::ValidationException("title", "Field is missing");
  } else {
    ValidateTitle(dto.title.value());
  }

  if (!dto.description) {
    throw utils::error::ValidationException("description", "Field is missing");
  } else {
    ValidateTitle(dto.description.value());
  }

  if (!dto.body) {
    throw utils::error::ValidationException("body", "Field is missing");
  } else {
    ValidateTitle(dto.body.value());
  }

  if (dto.tags) {
    ValidateTags(dto.tags.value());
  }
}

}  // namespace real_medium::validator