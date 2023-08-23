#include "profiles_follow_delete.hpp"
#include "db/sql.hpp"
#include "dto/profile.hpp"
#include "models/profile.hpp"
#include "userver/formats/yaml/value_builder.hpp"
#include "userver/server/handlers/http_handler_base.hpp"
#include "userver/storages/postgres/cluster.hpp"
#include "userver/storages/postgres/component.hpp"
#include "utils/make_error.hpp"

using namespace std;
using namespace userver::formats;
using namespace userver::server::http;
using namespace userver::server::request;
using namespace userver::storages::postgres;

namespace real_medium::handlers::profiles::del {

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
  auto user_id = context.GetData<std::optional<std::string>>("id");
  const auto& username = request.GetPathArg("username");
  if (username.empty()) {
    auto& response = request.GetHttpResponse();
    response.SetStatus(userver::server::http::HttpStatus::kNotFound);
    return utils::error::MakeError("username", "It is null.");
  }

  const auto res_find_id_username =
      pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kSlave,
                           sql::kFindUserIDByUsername.data(), username);
  if (res_find_id_username.IsEmpty()) {
    auto& response = request.GetHttpResponse();
    response.SetStatus(userver::server::http::HttpStatus::kNotFound);
    return utils::error::MakeError("username",
                                   "There is no user with this nickname.");
  }

  std::string username_id = res_find_id_username.AsSingleRow<std::string>();
  if (username_id == user_id) {
    auto& response = request.GetHttpResponse();
    response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
    return utils::error::MakeError("username",
                                   "Username is author of the request.");
  }

  const auto res_unfollowing =
      pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kSlave,
                           sql::KUnFollowingUser.data(), username_id, user_id);

  const auto profile =
      res_unfollowing.AsSingleRow<real_medium::models::Profile>(
          userver::storages::postgres::kRowTag);

  if (profile.isFollowing) {
    auto& response = request.GetHttpResponse();
    response.SetStatus(userver::server::http::HttpStatus::kBadRequest);
    return utils::error::MakeError("user_id", "has already unfollowed");
  }

  userver::formats::json::ValueBuilder builder;
  builder["profile"] = profile;

  return builder.ExtractValue();
}

}  // namespace real_medium::handlers::profiles::del
