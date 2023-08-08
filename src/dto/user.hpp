#pragma once

#include <string>
#include <tuple>

#include <userver/formats/json.hpp>

namespace real_medium::dto {

struct UserRegistrationDTO {
  std::string username;
  std::string email;
  std::string password;
};

struct UserLoginDTO {
  std::string email;
  std::string password;
};

UserRegistrationDTO Parse(const userver::formats::json::Value& json,
           userver::formats::parse::To<UserRegistrationDTO>);

UserLoginDTO Parse(const userver::formats::json::Value& json,
           userver::formats::parse::To<UserLoginDTO>);
}