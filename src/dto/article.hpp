#pragma once

#include <string>
#include <userver/storages/postgres/io/chrono.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/json/value.hpp>
#include "../models/article.hpp"
#include "../validators/length_validator.hpp"
#include "profile.hpp"

namespace real_medium::dto {

struct Article final {
  static Article Parse(const models::TaggedArticleWithProfile& model);

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
  static constexpr int MIN_BODY_LEN=5;
  static constexpr int MIN_DESCR_LEN=5;
  static constexpr int MIN_TAG_NAME_LEN=2;

  static constexpr int MAX_TITLE_LEN=256;
  static constexpr int MAX_BODY_LEN=65535;
  static constexpr int MAX_DESCR_LEN=8192;
  static constexpr int MAX_TAG_NAME_LEN=256;
  static CreateArticleRequest Parse(const userver::formats::json::Value& data);
  std::string title;
  std::string description;
  std::string body;
  std::optional<std::vector<std::string>> tags;
};


}  // namespace realmedium::dto
