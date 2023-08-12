#include "profile.hpp"
#include "userver/formats/json/value_builder.hpp"

namespace real_medium::dto {

userver::formats::json::Value Serialize(
    const real_medium::dto::Profile& data,
    userver::formats::serialize::To<userver::formats::json::Value>) {
  userver::formats::json::ValueBuilder builder;
  builder["username"] = data.username;
  if (data.bio) {
    builder["bio"] = *data.bio;
  } else {
    builder["bio"] = userver::formats::common::Type::kNull;
  }
  if (data.image) {
    builder["image"] = *data.image;
  } else {
    builder["image"] = userver::formats::common::Type::kNull;
  }
  builder["following"] = data.following;
  return builder.ExtractValue();
}

}  // namespace real_medium::dto