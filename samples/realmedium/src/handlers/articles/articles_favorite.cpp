#include "articles_favorite.hpp"
#include "db/sql.hpp"
#include "dto/article.hpp"

namespace real_medium::handlers::articles_favorite::post {

Handler::Handler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& component_context)
    : HttpHandlerJsonBase(config, component_context),
      pg_cluster_(component_context
                      .FindComponent<userver::components::Postgres>(
                          "realmedium-database")
                      .GetCluster()) {}

userver::formats::json::Value Handler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest& request,
    const userver::formats::json::Value&,
    userver::server::request::RequestContext& context) const {
  auto& user_id = context.GetData<std::optional<std::string>>("id");
  auto& slug = request.GetPathArg("slug");

  auto transaction =
      pg_cluster_->Begin("favorite_article_transaction",
                         userver::storages::postgres::ClusterHostType::kMaster,
                         userver::storages::postgres::Transaction::RW);

  auto res =
      transaction.Execute(sql::kInsertFavoritePair.data(), user_id, slug);

  if (!res.IsEmpty()) {
    auto article_id = res.AsSingleRow<std::string>();

    transaction.Execute(sql::kIncrementFavoritesCount.data(), article_id);
    transaction.Commit();
  }

  const auto get_article_res = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kSlave,
      real_medium::sql::kGetArticleWithAuthorProfileBySlug.data(), slug,
      user_id);

  if (get_article_res.IsEmpty()) {
    request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
    return {};
  }

  userver::formats::json::ValueBuilder builder;
  builder["article"] =
      get_article_res
          .AsSingleRow<real_medium::models::TaggedArticleWithProfile>();
  return builder.ExtractValue();
}

}  // namespace real_medium::handlers::articles_favorite::post