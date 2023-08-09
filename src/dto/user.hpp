#pragma once

#include <string>
#include <tuple>

#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>
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

struct UserUpdateDTO {
  std::optional<std::string> email;
  std::optional<std::string> username;
  std::optional<std::string> password;
  std::optional<std::string> bio;
  std::optional<std::string> image;
};

UserRegistrationDTO Parse(const userver::formats::json::Value& json,
           userver::formats::parse::To<UserRegistrationDTO>);

UserLoginDTO Parse(const userver::formats::json::Value& json,
           userver::formats::parse::To<UserLoginDTO>);


UserUpdateDTO Parse(const userver::formats::json::Value& json,
           userver::formats::parse::To<UserUpdateDTO>);
}