#pragma once

#include <optional>
#include <string>
#include <tuple>

#include <userver/formats/json/value_builder.hpp>
#include <userver/storages/postgres/io/io_fwd.hpp>
#include <userver/storages/postgres/io/pg_types.hpp>
#include "db/types.hpp"

namespace real_medium::models {

using UserId = std::string;
struct User final {
  UserId id;
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

}  // namespace real_medium::models

namespace userver::storages::postgres::io {

template <>
struct CppToUserPg<real_medium::models::User> {
  static constexpr DBTypeName postgres_name{
      real_medium::sql::types::kUser.data()};
};

}  // namespace userver::storages::postgres::io
