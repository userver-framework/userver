#include "article.hpp"
#include <userver/formats/serialize/common_containers.hpp>

namespace real_medium::models {

userver::formats::json::Value Serialize(
    const TaggedArticleWithProfile& article,
    userver::formats::serialize::To<userver::formats::json::Value>) {
  
  userver::formats::json::ValueBuilder item;
  
  item["article_id"] = article.articleId;
  item["title"] = article.title;
  item["slug"] = article.slug;
  item["body"] = article.body;
  item["description"] = article.description;
  item["created_at"] = article.createdAt;
  item["updated_at"] = article.updatedAt;
  item["favorited"] = article.isFavorited;
  item["favorites_count"] = article.favoritesCount;
  item["author"] = article.authorProfile;
  
  return item.ExtractValue();
}

}  // namespace real_medium::models
