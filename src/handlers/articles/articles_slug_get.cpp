#include "articles_slug_get.hpp"
#include "../../db/sql.hpp"
#include "../../dto/article.hpp"
#include "../../models/article.hpp"

namespace real_medium::handlers::articles_slug::get {
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
    const userver::formats::json::Value& /*request_json*/,
    userver::server::request::RequestContext& context) const {
  const auto& slug = request.GetPathArg("slug");
  const std::optional<std::string> userId =
      context.GetData<std::optional<std::string>>("id");
  auto data=cache_.Get();
  auto article=data->findArticleBySlug(slug);
  if(article==nullptr){
    request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
    return {};
  }
  userver::formats::json::ValueBuilder builder;
  builder["article"] = real_medium::dto::Article::Parse(*article,userId);
  return builder.ExtractValue();
}

}  // namespace real_medium::handlers::articles_slug::get
