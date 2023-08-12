#include "article.hpp"


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

CreateArticleRequest CreateArticleRequest::Parse(const userver::formats::json::Value& data) {
  CreateArticleRequest createArticleReq;

  createArticleReq.title = real_medium::validators::CheckLength(data, "title", MIN_TITLE_LEN, MAX_TITLE_LEN);
  createArticleReq.body = real_medium::validators::CheckLength(data, "body", MIN_BODY_LEN, MAX_BODY_LEN);
  createArticleReq.description = real_medium::validators::CheckLength(data, "description", MIN_DESCR_LEN, MAX_DESCR_LEN);
  createArticleReq.tags =  data["tagList"].As<std::optional<std::vector<std::string>>>();
  if (createArticleReq.tags)
    for(const auto &tag:*createArticleReq.tags)
      real_medium::validators::CheckLength(tag,"tagList",MIN_TAG_NAME_LEN,MAX_TAG_NAME_LEN);
  return createArticleReq;
}

}  // realmedium::dto
