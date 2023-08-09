#include "article.hpp"
#include <userver/formats/serialize/common_containers.hpp>

namespace real_medium::models {

userver::formats::json::Value Serialize(
    const Article& article,
    userver::formats::serialize::To<userver::formats::json::Value>) {
  
  userver::formats::json::ValueBuilder item;
  
  item["article_id"] = article.article_id;
  item["title"] = article.title;
  item["slug"] = article.slug;
  item["body"] = article.body;
  item["description"] = article.description;
  item["created_at"] = article.created_at;
  item["updated_at"] = article.updated_at;
  item["favorites_count"] = article.favorites_count;
  item["user_id"] = article.user_id;
  
  return item.ExtractValue();
}

}  // namespace real_medium::models
