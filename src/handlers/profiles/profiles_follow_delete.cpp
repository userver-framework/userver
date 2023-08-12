#include "db/sql.hpp"
#include "dto/profile.hpp"
#include "models/profile.hpp"
#include "profiles_follow_delete.hpp"
#include "userver/formats/yaml/value_builder.hpp"
#include "userver/server/handlers/http_handler_base.hpp"
#include "userver/storages/postgres/cluster.hpp"
#include "userver/storages/postgres/component.hpp"

using namespace std;
using namespace userver::formats;
using namespace userver::server::http;
using namespace userver::server::request;
using namespace userver::server::http;
using namespace userver::storages::postgres;

namespace real_medium::handlers::profiles::del {

Handler::Handler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& component_context)
    : HttpHandlerJsonBase(config, component_context),
      cluster_(component_context
                   .FindComponent<userver::components::Postgres>(
                       "realmedium-database")
                   .GetCluster()) {}

userver::formats::json::Value Handler::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest& request,
    const userver::formats::json::Value&,
    userver::server::request::RequestContext& request_context) const {
  const auto& username = request.GetPathArg("username");
  if (username.empty()) {
    request.SetResponseStatus(HttpStatus::kBadRequest);
    return {};
  }
  auto userId = request_context.GetData<std::string>("id");
  const auto res = cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kSlave, sql::kGetProfileByUsername.data(), username, userId);
  if (res.IsEmpty()) {
    request.SetResponseStatus(userver::server::http::HttpStatus::kNotFound);
    return {};
  }
  const auto user = res.AsSingleRow<models::Profile>();
  cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      sql::kUnfollowUser.data(), userId);
  userver::formats::json::ValueBuilder builder;
  builder["profile"] =
      dto::Profile{user.username, user.bio, user.image, false};
  return builder.ExtractValue();
}

}  // namespace real_medium::handlers::profiles::del
