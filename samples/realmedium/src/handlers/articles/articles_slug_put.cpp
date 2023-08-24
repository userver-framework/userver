#include "articles_slug_put.hpp"
#include "db/sql.hpp"
#include "dto/article.hpp"
#include "models/article.hpp"
#include "utils/errors.hpp"
#include "utils/slugify.hpp"
#include "validators/validators.hpp"

namespace real_medium::handlers::articles_slug::put {
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
  auto slug = request.GetPathArg("slug");
  real_medium::dto::UpdateArticleRequest updateRequest;
  try {
    updateRequest = real_medium::dto::UpdateArticleRequest::Parse(request_json["article"], request);
  } catch (const real_medium::utils::error::ValidationException& ex) {
    request.SetResponseStatus(
        userver::server::http::HttpStatus::kUnprocessableEntity);
    return ex.GetDetails();
  }
  auto userId = context.GetData<std::optional<std::string>>("id");

  std::string articleId;
  try {
    const auto newSlug =
        updateRequest.title
            ? std::make_optional<std::string>(
                  real_medium::utils::slug::Slugify(*updateRequest.title))
            : std::nullopt;
    const auto res = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        real_medium::sql::kUpdateArticleBySlug.data(), slug, userId,
        updateRequest.title, newSlug, updateRequest.description,
        updateRequest.body);
    if (res.IsEmpty()) {
      request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
      return {};
    }
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
  builder["article"] =
      res.AsSingleRow<real_medium::models::TaggedArticleWithProfile>();
  return builder.ExtractValue();
}
}  // namespace real_medium::handlers::articles_slug::put
