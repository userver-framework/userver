#pragma once

#include <string>
#include <optional>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>

namespace real_medium::dto {

struct Profile {
  std::string username;
  std::optional<std::string> bio;
  std::optional<std::string> image;
  bool isFollowing;
};

userver::formats::json::Value Serialize(
    const Profile& dto,
    userver::formats::serialize::To<userver::formats::json::Value>);

}  // namespace realmedium::dto
