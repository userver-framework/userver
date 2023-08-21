#include "article.hpp"
#include "validators/length_validator.hpp"

namespace real_medium::dto {
CreateArticleRequest Parse(const userver::formats::json::Value& json,
                           userver::formats::parse::To<CreateArticleRequest>) {
  return CreateArticleRequest{
      json["title"].As<std::optional<std::string>>(),
      json["description"].As<std::optional<std::string>>(),
      json["body"].As<std::optional<std::string>>(),
      json["tagList"].As<std::optional<std::vector<std::string>>>()};
}
UpdateArticleRequest Parse(const userver::formats::json::Value& json,
                           userver::formats::parse::To<UpdateArticleRequest>) {
  return UpdateArticleRequest{
      json["title"].As<std::optional<std::string>>(),
      json["description"].As<std::optional<std::string>>(),
      json["body"].As<std::optional<std::string>>()};
}

}  // namespace real_medium::dto
