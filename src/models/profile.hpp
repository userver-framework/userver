#pragma once

#include <optional>
#include <string>
#include <tuple>
#include <userver/formats/json/value_builder.hpp>
#include <userver/storages/postgres/io/io_fwd.hpp>
#include <userver/storages/postgres/io/pg_types.hpp>
#include "../db/types.hpp"

namespace real_medium::models {

struct Profile final {
  std::string username;
  std::optional<std::string> bio;
  std::optional<std::string> image;
  bool isFollowing{false};
  auto Introspect() { return std::tie(username, bio, image, isFollowing); }
};

userver::formats::json::Value Serialize(
    const Profile& profile,
    userver::formats::serialize::To<userver::formats::json::Value>);

}  // namespace real_medium::models

namespace userver::storages::postgres::io {

template <>
struct CppToUserPg<real_medium::models::Profile> {
  static constexpr DBTypeName postgres_name{
      real_medium::sql::types::kProfile.data()};
};

}  // namespace userver::storages::postgres::io
