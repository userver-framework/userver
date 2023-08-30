#include "user.hpp"
#include "utils/jwt.hpp"

#include <userver/formats/serialize/common_containers.hpp>

namespace real_medium::models {

userver::formats::json::Value Serialize(
    const User& user,
    userver::formats::serialize::To<userver::formats::json::Value>) {
  userver::formats::json::ValueBuilder item;
  item["email"] = user.email;
  item["username"] = user.username;
  item["token"] = utils::jwt::GenerateJWT(user.id);
  item["bio"] = user.bio;
  item["image"] = user.image;
  return item.ExtractValue();
}

}  // namespace real_medium::models
