#include "articles_get.hpp"
#include <userver/formats/serialize/common_containers.hpp>
#include "db/sql.hpp"
#include "dto/article.hpp"
#include "dto/filter.hpp"
#include "models/article.hpp"
#include "utils/make_error.hpp"

namespace real_medium::handlers::articles::get {

Handler::Handler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& component_context)
    : HttpHandlerJsonBase(config, component_context),
      pg_cluster_(component_context
                      .FindComponent<userver::components::Postgres>(
                          "realmedium-database")
                      .GetCluster()),
      cache_(component_context.FindComponent<
             real_medium::cache::articles_cache::ArticlesCache>()) {}

userver::formats::json::Value Handler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest& request,
    const userver::formats::json::Value& request_json,
    userver::server::request::RequestContext& context) const {
  dto::ArticleFilterDTO filter;

  try {
    filter = dto::Parse<dto::ArticleFilterDTO>(request);
  } catch (std::bad_cast& ex) {
    auto& response = request.GetHttpResponse();
    response.SetStatus(userver::server::http::HttpStatus::kUnprocessableEntity);
    return utils::error::MakeError("filters", "invalid filters entered");
  }

  auto user_id = context.GetData<std::optional<std::string>>("id");
  auto data = cache_.Get();
  auto recentArticles = data->getRecent(filter);
  userver::formats::json::ValueBuilder builder;
  builder["articles"] = userver::formats::common::Type::kArray;
  for (auto& article : recentArticles)
    builder["articles"].PushBack(dto::Article::Parse(*article, user_id));
  builder["articlesCount"] = recentArticles.size();
  return builder.ExtractValue();
}

}  // namespace real_medium::handlers::articles::get
