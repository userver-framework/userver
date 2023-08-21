#pragma once

#include <string>
#include <userver/storages/postgres/io/chrono.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/json/value.hpp>
#include "../models/article.hpp"
#include "../validators/length_validator.hpp"
#include "profile.hpp"

namespace real_medium::dto {

struct Article final {
  static Article Parse(const models::TaggedArticleWithProfile& model);
  static Article Parse(const models::FullArticleInfo&info, std::optional<std::string> authUserId);
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

userver::formats::json::Value Serialize(
    const Article& data,
    userver::formats::serialize::To<userver::formats::json::Value>);

struct CreateArticleRequest final {
  static constexpr int MIN_TITLE_LEN=3;
  static constexpr int MAX_TITLE_LEN=256;
  static constexpr int MIN_BODY_LEN=5;
  static constexpr int MAX_BODY_LEN=65535;
  static constexpr int MIN_DESCR_LEN=5;
  static constexpr int MAX_DESCR_LEN=8192;
  static constexpr int MIN_TAG_NAME_LEN=2;
  static constexpr int MAX_TAG_NAME_LEN=256;
  static CreateArticleRequest Parse(const userver::formats::json::Value& data);
  std::string title;
  std::string description;
  std::string body;
  std::optional<std::vector<std::string>> tags;
};

struct UpdateArticleRequest final {
  static constexpr int MIN_TITLE_LEN=3;
  static constexpr int MAX_TITLE_LEN=256;
  static constexpr int MIN_BODY_LEN=5;
  static constexpr int MAX_BODY_LEN=65535;
  static constexpr int MIN_DESCR_LEN=5;
  static constexpr int MAX_DESCR_LEN=8192;
  static constexpr int MIN_TAG_NAME_LEN=2;
  static constexpr int MAX_TAG_NAME_LEN=256;
  static UpdateArticleRequest Parse(
      const userver::formats::json::Value& data,
      const userver::server::http::HttpRequest& request) ;
  std::string slug;
  std::optional<std::string> title;
  std::optional<std::string> description;
  std::optional<std::string> body;
  std::optional<std::vector<std::string>> tags;
};

}  // namespace realmedium::dto
