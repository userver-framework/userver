#pragma once

#include <optional>
#include <string>
#include <tuple>

#include <userver/formats/json/value_builder.hpp>

namespace real_medium::models {

struct Profile {
  std::string username;
  std::optional<std::string> bio;
  std::optional<std::string> image;
  bool following{false};

  auto Introspect() {
    return std::tie(username, bio, image, following);
  }
};

userver::formats::json::Value Serialize(
    const Profile& profile,
    userver::formats::serialize::To<userver::formats::json::Value>);

}  // namespace real_medium::models

