#include "article.hpp"
#include "validators/length_validator.hpp"


namespace real_medium::dto {

Article Article::Parse(const models::TaggedArticleWithProfile& model) {
  Article article;
  article.slug = model.slug;
  article.title = model.title;
  article.description = model.description;
  article.body = model.body;
  article.tags = model.tags;
  article.createdAt = model.createdAt;
  article.updatedAt = model.updatedAt;
  article.favoritesCount = model.favoritesCount;
  article.isFavorited = model.isFavorited;
  article.profile.bio = model.authorProfile.bio;
  article.profile.image = model.authorProfile.image;
  article.profile.username = model.authorProfile.username;
  article.profile.isFollowing = model.authorProfile.isFollowing;
  return article;
}

userver::formats::json::Value Serialize(
    const Article& article,
    userver::formats::serialize::To<userver::formats::json::Value>) {
  userver::formats::json::ValueBuilder builder;
  builder["slug"] = article.slug;
  builder["title"] = article.title;
  builder["description"] = article.description;
  builder["body"] = article.body;
  builder["tagList"] = userver::formats::common::Type::kArray;
  if (article.tags) {
    std::for_each(
        article.tags->begin(), article.tags->end(),
        [&builder](const auto& tag) { builder["tagList"].PushBack(tag); });
  }
  builder["createdAt"] = article.createdAt;
  builder["updatedAt"] = article.updatedAt;
  builder["favoritesCount"] = article.favoritesCount;
  builder["favorited"] = article.isFavorited;
  builder["author"] = article.profile;
  return builder.ExtractValue();
}

CreateArticleRequest Parse(const userver::formats::json::Value& json,
                           userver::formats::parse::To<CreateArticleRequest>) {
  return CreateArticleRequest{
    json["title"].As<std::optional<std::string>>(),
        json["description"].As<std::optional<std::string>>(),
        json["body"].As<std::optional<std::string>>(),
        json["tagList"].As<std::optional<std::vector<std::string>>>()
  };
}
UpdateArticleRequest Parse(
    const userver::formats::json::Value& json,
    userver::formats::parse::To<UpdateArticleRequest>) {
  return UpdateArticleRequest{
      json["title"].As<std::optional<std::string>>(),
      json["description"].As<std::optional<std::string>>(),
      json["body"].As<std::optional<std::string>>()
  };
}

}  // realmedium::dto
