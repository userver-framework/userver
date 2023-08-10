#pragma once

#include <optional>
#include <string>
#include <tuple>

#include <userver/formats/json/value_builder.hpp>
#include <userver/storages/postgres/io/chrono.hpp>


namespace real_medium::models {

struct Comment {
  int id;
  userver::storages::postgres::TimePointTz created_at;
  userver::storages::postgres::TimePointTz updated_at;
  std::string body;
  std::string user_id;
  std::string article_id;


  auto Introspect() {
    return std::tie(id, created_at, updated_at, body,
    user_id, article_id);
  }
};

userver::formats::json::Value Serialize(
    const Comment& comment,
    userver::formats::serialize::To<userver::formats::json::Value>);

}  // namespace realworld::models
