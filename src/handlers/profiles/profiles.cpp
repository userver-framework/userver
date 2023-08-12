#include "profiles.hpp"
#include <string>
#include "dto/profile.hpp"
#include "db/sql.hpp"
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
using namespace real_medium::dto;

namespace real_medium::handlers::profiles::get {

Handler::Handler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& component_context)
    : HttpHandlerJsonBase(config, component_context),
      cluster_(component_context
                   .FindComponent<userver::components::Postgres>(
                       "realmedium-database")
                   .GetCluster()) {}

json::Value Handler::HandleRequestJsonThrow(
    const HttpRequest& request, const json::Value&,
    RequestContext& request_context) const {
  const auto& username = request.GetPathArg("username");
  if (username.empty()) {
    request.SetResponseStatus(HttpStatus::kBadRequest);
    return {};
  }
  auto userId = request_context.GetData<std::string>("id");

  auto res = cluster_->Execute(ClusterHostType::kSlave, real_medium::sql::kGetProfileByUsername, username, userId);
  if (res.IsEmpty()) {
    request.SetResponseStatus(HttpStatus::kNotFound);
    return {};
  }
  const auto profile = res.AsSingleRow<Profile>();
  userver::formats::json::ValueBuilder builder;
  builder["profile"] =
      Profile{profile.username, profile.bio, profile.image, profile.following};
  return builder.ExtractValue();
}

}  // namespace real_medium::handlers::profiles::get
