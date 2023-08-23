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

Article Article::Parse(const models::FullArticleInfo& model, std::optional<std::string> authUserId){
  Article article;
  article.slug = model.slug;
  article.title = model.title;
  article.description = model.description;
  article.body = model.body;
  if(!model.tags.empty())
    article.tags = std::vector<std::string>(model.tags.begin(),model.tags.end());
  article.createdAt = model.createdAt;
  article.updatedAt = model.updatedAt;
  article.favoritesCount = model.articleFavoritedByUsernames.size();
  article.isFavorited = authUserId?
      model.articleFavoritedByUserIds.find(authUserId.value())!=model.articleFavoritedByUserIds.end(): false;
  article.profile.bio = model.authorInfo.bio;
  article.profile.image = model.authorInfo.image;
  article.profile.username = model.authorInfo.username;
  article.profile.isFollowing = authUserId?
      model.authorFollowedByUsersIds.find(authUserId.value())!=model.authorFollowedByUsersIds.end(): false;
  return article;
}

CreateArticleRequest CreateArticleRequest::Parse(const userver::formats::json::Value& json) {
  return CreateArticleRequest{
      json["title"].As<std::optional<std::string>>(),
      json["description"].As<std::optional<std::string>>(),
      json["body"].As<std::optional<std::string>>(),
      json["tagList"].As<std::optional<std::vector<std::string>>>()};

}

UpdateArticleRequest UpdateArticleRequest::Parse(
    const userver::formats::json::Value& json,
    const userver::server::http::HttpRequest& request) {
  return UpdateArticleRequest{
      json["title"].As<std::optional<std::string>>(),
      json["description"].As<std::optional<std::string>>(),
      json["body"].As<std::optional<std::string>>()};

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



}  // realmedium::dto
