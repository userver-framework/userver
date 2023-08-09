#pragma once

#include <optional>
#include <string>
#include <tuple>

#include <userver/formats/json/value_builder.hpp>

namespace real_medium::models {

struct Comment {
  std::string comment_id;
  std::string created_at;
  std::string updated_at;
  std::string body;
  std::string user_id;
  std::string article_id;


  auto Introspect() {
    return std::tie(comment_id, created_at, updated_at, body,
    user_id, article_id);
  }
};

userver::formats::json::Value Serialize(
    const Comment& comment,
    userver::formats::serialize::To<userver::formats::json::Value>);

}  // namespace realworld::models
