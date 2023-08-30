
#include "comment_delete.hpp"
#include "db/sql.hpp"
#include "utils/make_error.hpp"

namespace real_medium::handlers::comments::del {

Handler::Handler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& component_context)
    : HttpHandlerJsonBase(config, component_context),
      pg_cluster_(component_context
                      .FindComponent<userver::components::Postgres>(
                          "realmedium-database")
                      .GetCluster()) {}

userver::formats::json::Value Handler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest& request,
    const userver::formats::json::Value& /*request_json*/,
    userver::server::request::RequestContext& context) const {
  auto user_id = context.GetData<std::optional<std::string>>("id");
  const auto& comment_id = std::atoi(request.GetPathArg("id").c_str());
  const auto& slug = request.GetPathArg("slug");

  const auto result_find_comment = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      sql::kFindCommentByIdAndSlug.data(), comment_id, slug);

  if (result_find_comment.IsEmpty()) {
    auto& response = request.GetHttpResponse();
    response.SetStatus(userver::server::http::HttpStatus::kNotFound);
    return utils::error::MakeError("comment_id", "Invalid comment_id.");
  }

  const auto result_delete_comment = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      sql::kDeleteCommentById.data(), comment_id, user_id);

  if (result_delete_comment.IsEmpty()) {
    auto& response = request.GetHttpResponse();
    response.SetStatus(userver::server::http::HttpStatus::kForbidden);
    return utils::error::MakeError("user_id",
                                   "This user does not own this comment.");
  }

  return {};
}

}  // namespace real_medium::handlers::comments::del
