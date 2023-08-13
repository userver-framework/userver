#include "comment.hpp"
#include <userver/formats/serialize/common_containers.hpp>

namespace real_medium::models {

userver::formats::json::Value Serialize(
    const Comment& comment,
    userver::formats::serialize::To<userver::formats::json::Value>) {
  userver::formats::json::ValueBuilder item;

  item["id"] = comment.id;
  item["created_at"] = comment.created_at;
  item["updated_at"] = comment.updated_at;
  item["body"] = comment.body;
  item["author"] = comment.author;

  return item.ExtractValue();
}

}  // namespace real_medium::models
