#pragma once

#include <string>

#include <userver/formats/json/value.hpp>

namespace real_medium::dto {

struct Profile {
  std::string username;
  std::string bio;
  std::string image;
  bool following;
};

userver::formats::json::Value Serialize(
    const Profile& dto,
    userver::formats::serialize::To<userver::formats::json::Value>);

}  // namespace real_medium::dto