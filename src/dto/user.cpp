#include "user.hpp"

namespace real_medium::dto {

UserRegistrationDTO Parse(const userver::formats::json::Value& json,
                 userver::formats::parse::To<UserRegistrationDTO>) {
  return UserRegistrationDTO{
      json["username"].As<std::string>(),
      json["email"].As<std::string>(),
      json["password"].As<std::string>(),
  };
}

UserLoginDTO Parse(const userver::formats::json::Value& json,
                 userver::formats::parse::To<UserLoginDTO>) {
  return UserLoginDTO{
      json["email"].As<std::string>(),
      json["password"].As<std::string>(),
  };
}

}