#include "articles_post.hpp"
#include <userver/logging/log.hpp>

#include "../../dto/article.hpp"
#include "../../utils/slugify.hpp"
#include "../../utils/slugify.hpp"
#include "../../db/sql.hpp"
#include "../../utils/errors.hpp"
#include "../../models/article.hpp"
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
    userver::server::request::RequestContext& request_context) const {
  dto::CreateArticleRequest createArticleRequest;
  try {
    createArticleRequest = dto::CreateArticleRequest::Parse(request_json["article"]);
  } catch (const real_medium::utils::error::ValidationException& ex) {
    request.SetResponseStatus(userver::server::http::HttpStatus::kUnprocessableEntity);
    return ex.ToJson();
  }

  auto userId = request_context.GetData<std::string>("id");

  std::string articleId;
  try {
    const auto slug = real_medium::utils::slug::Slugify(createArticleRequest.title);

    const auto res = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        real_medium::sql::kCreateArticle.data(), createArticleRequest.title, slug,
        createArticleRequest.body, createArticleRequest.description, userId, createArticleRequest.tags);

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

  LOG_DEBUG() << "------------------article_id="<<articleId<<"-------------------";
  const auto res =
      pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kMaster,
                        real_medium::sql::kGetArticleWithAuthorProfile.data(),
                        articleId, userId);
  LOG_DEBUG() << "++++++++++++++++++++article_id="<<articleId<<"++++++++++++++++";
  LOG_DEBUG() << "**********size="<<res.Size()<<"***************";


  userver::formats::json::ValueBuilder builder;
  builder["article"] =
      dto::Article::Parse(res.AsSingleRow<real_medium::models::TaggedArticleWithProfile>());
  return builder.ExtractValue();
}
} // namespace real_medium::handlers::articles::post
