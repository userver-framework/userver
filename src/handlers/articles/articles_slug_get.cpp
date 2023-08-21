#include "articles_slug_get.hpp"
#include "../../db/sql.hpp"
#include "../../dto/article.hpp"
#include "../../models/article.hpp"

namespace real_medium::handlers::articles_slug::get {
Handler::Handler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context)
    : HttpHandlerJsonBase(config, context),
      pg_cluster_(context
                      .FindComponent<userver::components::Postgres>(
                          "realmedium-database")
                      .GetCluster()) {}

userver::formats::json::Value Handler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest& request,
    const userver::formats::json::Value& request_json,
    userver::server::request::RequestContext& context) const {
  const auto& slug = request.GetPathArg("slug");
  const std::optional<std::string> userId =
      context.GetData<std::optional<std::string>>("id");
  const auto res = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kSlave,
      real_medium::sql::kGetArticleWithAuthorProfileBySlug.data(), slug,
      userId);
  if (res.IsEmpty()) {
    request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
    return {};
  }
  userver::formats::json::ValueBuilder builder;
  builder["article"] =
      res.AsSingleRow<real_medium::models::TaggedArticleWithProfile>();
  return builder.ExtractValue();
}

}  // namespace real_medium::handlers::articles_slug::get
