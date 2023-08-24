#include "articles_slug_delete.hpp"
#include "../../db/sql.hpp"
#include "../../dto/article.hpp"
#include "../../models/article.hpp"
#include "../../utils/slugify.hpp"

namespace real_medium::handlers::articles_slug::del {
Handler::Handler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& context)
    : HttpHandlerJsonBase(config, context),
      pg_cluster_(context
                      .FindComponent<userver::components::Postgres>(
                          "realmedium-database")
                      .GetCluster()) {}

userver::formats::json::Value Handler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest& request,
    const userver::formats::json::Value&,
    userver::server::request::RequestContext& context) const {
  const auto& slug = request.GetPathArg("slug");
  const auto userId = context.GetData<std::optional<std::string>>("id");
  auto res = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      real_medium::sql::kGetArticleIdBySlug.data(), slug);
  if (res.IsEmpty()) {
    request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
    return {};
  }
  res = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      real_medium::sql::kDeleteArticleBySlug.data(), slug, userId);

  if (res.IsEmpty()) {
    request.SetResponseStatus(userver::server::http::HttpStatus::kForbidden);
    return {};
  }

  return {};
}

}  // namespace real_medium::handlers::articles_slug::del
