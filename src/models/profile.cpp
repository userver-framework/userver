#include "profile.hpp"
#include <userver/formats/serialize/common_containers.hpp>

namespace real_medium::models {

userver::formats::json::Value Serialize(
    const Profile& profile,
    userver::formats::serialize::To<userver::formats::json::Value>);
  userver::formats::json::ValueBuilder item;

  item["username"] = profile.username;
  item["bio"] = profile.bio;
  item["image"] = profile.image;
  item["following"] = profile.following;

  return item.ExtractValue();

}  // namespace real_medium::models
