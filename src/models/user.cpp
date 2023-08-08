#include "user.hpp"

namespace real_medium::models {

userver::formats::json::Value Serialize(
    const User& user,
    userver::formats::serialize::To<userver::formats::json::Value>) {
  userver::formats::json::ValueBuilder item;
  item["email"] = user.email;
  item["username"] = user.username;
  item["token"] = "token";
  item["bio"] = user.bio;
  item["image"] = user.image;
  return item.ExtractValue();
}

}