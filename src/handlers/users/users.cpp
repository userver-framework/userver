#include <string>

#include "users.hpp"
#include "userver/server/handlers/http_handler_base.hpp"

#include "userver/storages/postgres/component.hpp"

#include <userver/crypto/hash.hpp>
#include "userver/components/component_config.hpp"
#include "userver/components/component_context.hpp"

#include "db/sql.hpp"
#include "models/user.hpp"
#include "validators/user_validators.hpp"

namespace real_medium::handlers::users::post {

RegisterUser::RegisterUser(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context),
      pg_cluster_(component_context
                      .FindComponent<userver::components::Postgres>(
                          "realmedium-database")
                      .GetCluster()) {}

std::string RegisterUser::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  auto request_body =
      userver::formats::json::FromString(request.RequestBody())["user"];

  auto username = request_body["username"].As<std::string>();
  auto email = request_body["email"].As<std::string>();
  auto password = request_body["password"].As<std::string>();

  if (!validators::ValidateEmail(email)) {
    auto& response = request.GetHttpResponse();
    response.SetStatus(userver::server::http::HttpStatus::kUnprocessableEntity);
    return {};
  }

  if (!validators::ValidatePassword(password)) {
    auto& response = request.GetHttpResponse();
    response.SetStatus(userver::server::http::HttpStatus::kUnprocessableEntity);
    return {};
  }

  auto hash_password = userver::crypto::hash::Sha256(password);
  models::User result_user;
  try {
    auto query_result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        sql::kInsertUser.data(), username, email, hash_password);
    result_user = query_result.AsSingleRow<models::User>(
        userver::storages::postgres::kRowTag);
  } catch (const userver::storages::postgres::UniqueViolation& ex) {
    auto& response = request.GetHttpResponse();
    response.SetStatus(userver::server::http::HttpStatus::kUnprocessableEntity);
    return "Constraint: " + ex.GetServerMessage().GetConstraint() +
           " message: " + ex.GetServerMessage().GetDetail();
  }

  userver::formats::json::ValueBuilder builder;
  builder["user"] = result_user;
  return userver::formats::json::ToString(builder.ExtractValue());
}

}  // namespace real_medium::handlers::users::post
