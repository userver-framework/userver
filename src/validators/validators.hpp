#pragma once

#include "dto/comment.hpp"
#include "dto/user.hpp"
#include "dto/article.hpp"

namespace real_medium::validator {

void validate(const dto::UserLoginDTO& dto);

void validate(const dto::UserRegistrationDTO& dto);

void validate(const dto::UserUpdateDTO& dto);

void validate(const dto::AddCommentDTO& dto);

void validate(const dto::CreateArticleRequest& dto);

}  // namespace real_medium::validator