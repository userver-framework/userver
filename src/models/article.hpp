#pragma once

#include <optional>
#include <string>
#include <tuple>
#include <userver/formats/json/value_builder.hpp>
#include <userver/storages/postgres/io/chrono.hpp>
#include <userver/storages/postgres/io/pg_types.hpp>
#include <vector>
#include "../db/types.hpp"
#include "profile.hpp"
#include "user.hpp"

namespace real_medium::models {
  using ArticleId=std::string;

struct FullArticleInfo{
    ArticleId articleId;
    std::string title;
    std::string slug;
    std::string description;
    std::string body;
    std::unordered_set<std::string> tags;
    userver::storages::postgres::TimePointTz createdAt;
    userver::storages::postgres::TimePointTz updatedAt;
    std::unordered_set<std::string> articleFavoritedByUserIds;
    std::unordered_set<std::string> articleFavoritedByUsernames;
    std::unordered_set<std::string> authorFollowedByUsersIds;
    User authorInfo;

    auto Introspect() {
      return std::tie(articleId, title, slug, description, body, tags,
                      createdAt,updatedAt,articleFavoritedByUserIds,
                      articleFavoritedByUsernames,authorFollowedByUsersIds,authorInfo);
    }
};

struct TaggedArticleWithProfile {
  ArticleId articleId;
  std::string title;
  std::string slug;
  std::string body;
  std::string description;
  userver::storages::postgres::TimePointTz createdAt;
  userver::storages::postgres::TimePointTz updatedAt;
  std::optional<std::vector<std::string>> tags;
  bool isFavorited;
  std::int64_t favoritesCount;
  Profile authorProfile;

  auto Introspect() {
    return std::tie(articleId, title, slug, body, description, createdAt,
                    updatedAt, tags, isFavorited, favoritesCount,
                    authorProfile);
  }
};

userver::formats::json::Value Serialize(
    const TaggedArticleWithProfile& article,
    userver::formats::serialize::To<userver::formats::json::Value>);

}  // namespace real_medium::models

namespace userver::storages::postgres::io {

template <>
struct CppToUserPg<real_medium::models::TaggedArticleWithProfile> {
  static constexpr DBTypeName postgres_name{
      real_medium::sql::types::kTaggedArticleWithProfile.data()};
};

template <>
struct CppToUserPg<real_medium::models::FullArticleInfo> {
  static constexpr DBTypeName postgres_name{
    real_medium::sql::types::kFullArticleInfo.data()
  };
};

} // namespace userver::storages::postgres::io
