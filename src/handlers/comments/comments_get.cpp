
#include "comments_get.hpp"
#include "db/sql.hpp"
#include "dto/comment.hpp"
#include "dto/profile.hpp"
#include <userver/formats/serialize/common_containers.hpp>
#include "utils/make_error.hpp"


namespace real_medium::handlers::comments::get {

Handler::Handler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& component_context)
    : HttpHandlerJsonBase(config, component_context),
      pg_cluster_(component_context
                      .FindComponent<userver::components::Postgres>(
                          "realmedium-database")
                      .GetCluster()),
      cache_(component_context.FindComponent<
             real_medium::cache::comments_cache::CommentsCache>()){}

userver::formats::json::Value Handler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest& request,
    const userver::formats::json::Value& request_json,
    userver::server::request::RequestContext& context) const {
  auto user_id = context.GetData<std::optional<std::string>>("id");
  const auto& slug = request.GetPathArg("slug");
  const auto data = cache_.Get();
  userver::formats::json::ValueBuilder result = userver::formats::json::MakeObject();
  
  const auto res_find_article = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kSlave,
      sql::kFindIdArticleBySlug.data(), slug);

  if (res_find_article.IsEmpty()) {
    auto& response = request.GetHttpResponse();
    response.SetStatus(userver::server::http::HttpStatus::kNotFound);
    return utils::error::MakeError("article_id", "Invalid article_id.");
  }

  const auto article_id = res_find_article.AsSingleRow<std::string>();
  const auto res_find_comments = data->findComments(article_id);

  userver::formats::json::ValueBuilder builder;
  builder["comments"] = userver::formats::common::Type::kArray;
  for (auto& comment : res_find_comments)
    builder["comments"].PushBack(dto::Comment::Parse(*comment, user_id));

  return builder.ExtractValue();
}

}  // namespace real_medium::hancleardlers::comments::get