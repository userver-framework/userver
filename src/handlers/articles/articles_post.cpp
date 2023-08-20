#include "articles_post.hpp"
#include <userver/logging/log.hpp>

#include "../../db/sql.hpp"
#include "../../dto/article.hpp"
#include "../../models/article.hpp"
#include "../../utils/errors.hpp"
#include "../../utils/slugify.hpp"
#include "validators/validators.hpp"
namespace real_medium::handlers::articles::post {

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
  auto createArticleRequest = request_json["article"].As<dto::CreateArticleRequest>();
  try {
    validator::validate(createArticleRequest);
  } catch (const real_medium::utils::error::ValidationException& ex) {
    // userver doesn't yet support 422 HTTP error code, so we handle the
    // exception by ourselves. In general the exceptions are processed by the
    // framework
    request.SetResponseStatus(
        userver::server::http::HttpStatus::kUnprocessableEntity);
    return ex.GetDetails();
  }

  const auto userId = context.GetData<std::optional<std::string>>("id");

  std::string articleId;
  try {
    const auto slug =
        real_medium::utils::slug::Slugify(createArticleRequest.title.value());

    const auto res = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        real_medium::sql::kCreateArticle.data(), createArticleRequest.title,
        slug, createArticleRequest.body, createArticleRequest.description,
        userId, createArticleRequest.tags);

    articleId = res.AsSingleRow<std::string>();
  } catch (const userver::storages::postgres::UniqueViolation& ex) {
    const auto constraint = ex.GetServerMessage().GetConstraint();
    if (constraint == "uniq_slug") {
      request.SetResponseStatus(
          userver::server::http::HttpStatus::kUnprocessableEntity);
      return real_medium::utils::error::MakeError("slug", "already exists");
    }
    throw;
  }

  const auto res = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      real_medium::sql::kGetArticleWithAuthorProfile.data(), articleId, userId);

  userver::formats::json::ValueBuilder builder;
  builder["article"] = dto::Article::Parse(
      res.AsSingleRow<real_medium::models::TaggedArticleWithProfile>());
  return builder.ExtractValue();
}
}  // namespace real_medium::handlers::articles::post
