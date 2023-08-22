#pragma once

#include <string>
#include <tuple>

#include <userver/formats/json.hpp>
#include <userver/formats/parse/common_containers.hpp>

#include "profile.hpp"
#include "models/comment.hpp"

namespace real_medium::dto {

struct Comment final{
  static Comment Parse(const real_medium::models::CachedComment& cachedComment, std::optional<std::string> userId);
  std::string id;
  userver::storages::postgres::TimePointTz createdAt;
  userver::storages::postgres::TimePointTz updatedAt;
  std::string body;
  Profile author;
};

struct AddComment {
  std::string body;
};

AddComment Parse(const userver::formats::json::Value& json,
                    userver::formats::parse::To<AddComment>);

userver::formats::json::Value Serialize(
    const Comment& comment,
    userver::formats::serialize::To<userver::formats::json::Value>);

}  // namespace real_medium::dto