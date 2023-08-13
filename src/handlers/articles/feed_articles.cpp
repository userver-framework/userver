#include "feed_articles.hpp"
#include <boost/lexical_cast.hpp>
#include <sstream>
#include <userver/formats/serialize/common_containers.hpp>
#include "db/sql.hpp"
#include "dto/article.hpp"
#include "models/article.hpp"

namespace real_medium::handlers::articles::feed::get {

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
  std::int32_t limit =
      request.HasArg("limit")
          ? boost::lexical_cast<int32_t>(request.GetArg("limit"))
          : 20;
  std::int32_t offset =
      request.HasArg("offset")
          ? boost::lexical_cast<int32_t>(request.GetArg("offset"))
          : 0;
  auto query_result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      sql::kFindArticlesByFollowedUsers.data(), user_id, limit, offset);
  auto articles = query_result.AsContainer<
      std::vector<real_medium::models::TaggedArticleWithProfile>>();
  userver::formats::json::ValueBuilder builder;
  builder["articles"] = userver::formats::common::Type::kArray;
  for (auto& article : articles) {
    builder["articles"].PushBack(dto::Article::Parse(article));
  }
  builder["articlesCount"] = articles.size();
  return builder.ExtractValue();
}

}  // namespace real_medium::handlers::articles::feed::get
