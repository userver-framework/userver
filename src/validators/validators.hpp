#pragma once

#include "dto/user.hpp"

namespace real_medium::validator {

void validate(const dto::UserLoginDTO& dto);

void validate(const dto::UserRegistrationDTO& dto);

}  // namespace realmedium::validator