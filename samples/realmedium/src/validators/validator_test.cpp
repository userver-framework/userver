#include <userver/utest/utest.hpp>

#include "user_validators.hpp"
#include "utils/errors.hpp"
#include "validators.hpp"

using namespace real_medium::validator;
using namespace real_medium::utils::error;

UTEST(UserValidation, EmailValidation) {
  EXPECT_TRUE(ValidateEmail("kek123@kek-lol.ru"));
  EXPECT_FALSE(ValidateEmail(""));
  EXPECT_FALSE(ValidateEmail("kek@lol.ru@lol.com"));
}

UTEST(UserValidation, PasswordValidation) {
  EXPECT_TRUE(ValidatePassword("keklol"));
  EXPECT_FALSE(ValidatePassword(""));
}

UTEST(UserValidation, UsernameValidation) {
  EXPECT_TRUE(ValidateUsername("kek"));
  EXPECT_FALSE(ValidateUsername(""));
}

UTEST(UserValidation, LoginValidation) {
  using real_medium::dto::UserLoginDTO;

  UEXPECT_NO_THROW(validate(UserLoginDTO{"kek@lol.ru", "keklol"}));
  UEXPECT_THROW(validate(UserLoginDTO{std::nullopt, "keklol"}),
                ValidationException);
  UEXPECT_THROW(validate(UserLoginDTO{"kek@lol.ru", std::nullopt}),
                ValidationException);
}

UTEST(UserValidation, RegisterValidation) {
  using real_medium::dto::UserRegistrationDTO;

  UEXPECT_NO_THROW(
      validate(UserRegistrationDTO{"kek", "kek@lol.ru", "keklol"}));
  UEXPECT_THROW(
      validate(UserRegistrationDTO{std::nullopt, "kek@lol.ru", "keklol"}),
      ValidationException);
  UEXPECT_THROW(validate(UserRegistrationDTO{"kek", std::nullopt, "keklol"}),
                ValidationException);
  UEXPECT_THROW(
      validate(UserRegistrationDTO{"kek", "kek@lol.ru", std::nullopt}),
      ValidationException);
}

UTEST(CommentValidation, CreateCommentValidation) {
  using real_medium::dto::AddComment;

  UEXPECT_NO_THROW(validate(AddComment{"some body"}));
  UEXPECT_THROW(validate(AddComment{std::nullopt}), ValidationException);
  UEXPECT_THROW(validate(AddComment{""}), ValidationException);
}

UTEST(ArticleValidation, CreateArticleValidation) {
  using real_medium::dto::CreateArticleRequest;

  UEXPECT_NO_THROW(validate(
      CreateArticleRequest{"title", "description", "some body", std::nullopt}));
  UEXPECT_NO_THROW(validate(CreateArticleRequest{
      "title", "description", "some body", std::vector<std::string>{}}));
  UEXPECT_NO_THROW(
      validate(CreateArticleRequest{"title", "description", "some body",
                                    std::vector<std::string>{"kek", "lol"}}));

  UEXPECT_THROW(
      validate(CreateArticleRequest{std::nullopt, "description", "some body",
                                    std::vector<std::string>{}}),
      ValidationException);
  UEXPECT_THROW(
      validate(CreateArticleRequest{"title", std::nullopt, "some body",
                                    std::vector<std::string>{}}),
      ValidationException);
  UEXPECT_THROW(
      validate(CreateArticleRequest{"title", "description", std::nullopt,
                                    std::vector<std::string>{}}),
      ValidationException);
  UEXPECT_THROW(
      validate(CreateArticleRequest{"title", "description", "some body",
                                    std::vector<std::string>{""}}),
      ValidationException);
}

UTEST(ArticleValidation, UpdateArticleValidation) {
  using real_medium::dto::UpdateArticleRequest;

  UEXPECT_NO_THROW(
      validate(UpdateArticleRequest{"title", "description", "some body"}));
  UEXPECT_NO_THROW(
      validate(UpdateArticleRequest{std::nullopt, std::nullopt, std::nullopt}));

  UEXPECT_THROW(
      validate(UpdateArticleRequest{std::nullopt, std::nullopt, "body"}),
      ValidationException);
}
