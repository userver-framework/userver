#include "profiles.hpp"
#include <string>
#include "db/sql.hpp"
#include "dto/profile.hpp"
#include "models/profile.hpp"
#include "utils/make_error.hpp"

#include "userver/formats/yaml/value_builder.hpp"
#include "userver/server/handlers/http_handler_base.hpp"
#include "userver/storages/postgres/cluster.hpp"
#include "userver/storages/postgres/component.hpp"

using namespace std;
using namespace userver::formats;
using namespace userver::server::http;
using namespace userver::server::request;
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

json::Value Handler::HandleRequestJsonThrow(const HttpRequest& request,
                                            const json::Value&,
                                            RequestContext& context) const {
  auto user_id = context.GetData<std::optional<std::string>>("id");
  const auto& username = request.GetPathArg("username");

  auto res =
      cluster_->Execute(ClusterHostType::kMaster,
                        sql::kGetProfileByUsername.data(), username, user_id);
  if (res.IsEmpty()) {
    auto& response = request.GetHttpResponse();
    response.SetStatus(userver::server::http::HttpStatus::kNotFound);
    return utils::error::MakeError("username",
                                   "There is no user with this nickname.");
  }

  auto profile = res.AsSingleRow<real_medium::models::Profile>(
      userver::storages::postgres::kRowTag);

  userver::formats::json::ValueBuilder builder;
  builder["profile"] = profile;

  return builder.ExtractValue();
}

}  // namespace real_medium::handlers::profiles::get
