#pragma once

#include <optional>
#include <string>
#include <tuple>
#include "models/profile.hpp"

#include <userver/formats/json/value_builder.hpp>
#include <userver/storages/postgres/io/chrono.hpp>

namespace real_medium::models {

struct Comment {
  int id;
  userver::storages::postgres::TimePointTz created_at;
  userver::storages::postgres::TimePointTz updated_at;
  std::string body;
  std::string user_id;
  real_medium::models::Profile author;

  auto Introspect() {
    return std::tie(id, created_at, updated_at, body);
  }
};

userver::formats::json::Value Serialize(
    const Comment& comment,
    userver::formats::serialize::To<userver::formats::json::Value>);

}  // namespace real_medium::models
