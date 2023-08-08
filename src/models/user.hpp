#pragma once

#include <optional>
#include <string>
#include <tuple>

#include <userver/formats/json/value_builder.hpp>

namespace real_medium::models {

struct User {
  std::string id;
  std::string username;
  std::string email;
  std::optional<std::string> bio;
  std::optional<std::string> image;
  std::string password_hash;

  auto Introspect() {
    return std::tie(id, username, email, bio, image, password_hash);
  }
};

userver::formats::json::Value Serialize(
    const User& user,
    userver::formats::serialize::To<userver::formats::json::Value>);

}  // namespace realworld::models