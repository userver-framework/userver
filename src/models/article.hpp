#pragma once

#include <optional>
#include <string>
#include <tuple>

#include <userver/formats/json/value_builder.hpp>

namespace real_medium::models {

struct Article {
  std::string article_id;
  std::string title;
  std::string slug;
  std::string body;
  std::string description;
  std::string created_at;
  std::string updated_at;
  int favorites_count;
  std::string user_id;

  auto Introspect() {
    return std::tie(article_id, title, slug, body, description, created_at,
    updated_at, favorites_count, user_id);
  }
};

userver::formats::json::Value Serialize(
    const Article& article,
    userver::formats::serialize::To<userver::formats::json::Value>);

}  // namespace realworld::models
