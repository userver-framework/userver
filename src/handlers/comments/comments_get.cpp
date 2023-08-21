
#include "comments_get.hpp"
#include "db/sql.hpp"
#include "dto/comment.hpp"
#include "models/comment.hpp"

#include "utils/make_error.hpp"

namespace real_medium::handlers::comments::get {

Handler::Handler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& component_context)
    : HttpHandlerJsonBase(config, component_context),
      pg_cluster_(component_context
                      .FindComponent<userver::components::Postgres>(
                          "realmedium-database")
                      .GetCluster()) {}

userver::formats::json::Value Handler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest& request,
    const userver::formats::json::Value& request_json,
    userver::server::request::RequestContext& context) const {
  auto user_id = context.GetData<std::optional<std::string>>("id");
  const auto& slug = request.GetPathArg("slug");

  const auto res_find_article = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      sql::kFindIdArticleBySlug.data(), slug);

  if (res_find_article.IsEmpty()) {
    auto& response = request.GetHttpResponse();
    response.SetStatus(userver::server::http::HttpStatus::kNotFound);
    return utils::error::MakeError("article_id", "Ivanlid article_id.");
  }

  const auto article_id = res_find_article.AsSingleRow<std::string>();

  const auto res_find_comments = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kSlave,
      sql::kFindCommentsByArticleId.data(), article_id, user_id);

  const auto comments =
      res_find_comments.AsContainer<std::vector<real_medium::models::Comment>>(
          userver::storages::postgres::kRowTag);

  userver::formats::json::ValueBuilder builder;
  builder["comments"] = comments;

  return builder.ExtractValue();
}

}  // namespace real_medium::handlers::comments::get