#include "article.hpp"
#include <userver/formats/serialize/common_containers.hpp>

namespace real_medium::models {

userver::formats::json::Value Serialize(
    const TaggedArticleWithProfile& article,
    userver::formats::serialize::To<userver::formats::json::Value>) {
  userver::formats::json::ValueBuilder item;

  item["slug"] = article.slug;
  item["title"] = article.title;
  item["description"] = article.description;
  item["body"] = article.body;
  item["tagList"] = article.tags;
  item["createdAt"] = article.createdAt;
  item["updatedAt"] = article.updatedAt;
  item["favorited"] = article.isFavorited;
  item["favoritesCount"] = article.favoritesCount;
  item["author"] = article.authorProfile;

  return item.ExtractValue();
}

}  // namespace real_medium::models
