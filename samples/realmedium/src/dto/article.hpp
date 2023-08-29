#pragma once

#include <string>
#include <userver/formats/json/value.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/storages/postgres/io/chrono.hpp>
#include "models/article.hpp"
#include "profile.hpp"

namespace real_medium::dto {
struct Article final {
  static Article Parse(const models::TaggedArticleWithProfile& model);
  static Article Parse(const models::FullArticleInfo& info,
                       std::optional<std::string> authUserId);
  std::string slug;
  std::string title;
  std::string body;
  std::string description;
  std::optional<std::vector<std::string>> tags;
  userver::storages::postgres::TimePointTz createdAt;
  userver::storages::postgres::TimePointTz updatedAt;
  std::int64_t favoritesCount{};
  bool isFavorited{false};
  Profile profile;
};

struct CreateArticleRequest final {
  static CreateArticleRequest Parse(const userver::formats::json::Value& json);
  std::optional<std::string> title;
  std::optional<std::string> description;
  std::optional<std::string> body;
  std::optional<std::vector<std::string>> tags;
};

struct UpdateArticleRequest final {
  static UpdateArticleRequest Parse(
      const userver::formats::json::Value& json,
      const userver::server::http::HttpRequest& request);
  std::optional<std::string> title;
  std::optional<std::string> description;
  std::optional<std::string> body;
};

userver::formats::json::Value Serialize(
    const Article& data,
    userver::formats::serialize::To<userver::formats::json::Value>);
}  // namespace real_medium::dto
