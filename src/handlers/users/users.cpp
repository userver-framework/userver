#include <string>

#include "users.hpp"

#include <userver/crypto/hash.hpp>

#include "db/sql.hpp"
#include "dto/user.hpp"
#include "models/user.hpp"
#include "utils/make_error.hpp"
#include "validators/user_validators.hpp"

namespace real_medium::handlers::users::post {

RegisterUser::RegisterUser(
    const userver::components::ComponentConfig& config,
    const userver::components::ComponentContext& component_context)
    : HttpHandlerJsonBase(config, component_context),
      pg_cluster_(component_context
                      .FindComponent<userver::components::Postgres>(
                          "realmedium-database")
                      .GetCluster()) {}

userver::formats::json::Value RegisterUser::HandleRequestJsonThrow(
    const userver::server::http::HttpRequest& request,
    const userver::formats::json::Value& request_json,
    userver::server::request::RequestContext& context) const {
  auto user_register =
      userver::formats::json::FromString(request.RequestBody())["user"]
          .As<dto::UserRegistrationDTO>();

  if (!validator::ValidateEmail(user_register.email)) {
    auto& response = request.GetHttpResponse();
    response.SetStatus(userver::server::http::HttpStatus::kUnprocessableEntity);
    return utils::error::MakeError("email", "Wrong email entered");
  }

  if (!validator::ValidatePassword(user_register.password)) {
    auto& response = request.GetHttpResponse();
    response.SetStatus(userver::server::http::HttpStatus::kUnprocessableEntity);
    return utils::error::MakeError("password", "Wrong password entered");
  }

  auto hash_password = userver::crypto::hash::Sha256(user_register.password);
  models::User result_user;
  try {
    auto query_result = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        sql::kInsertUser.data(), user_register.username, user_register.email,
        hash_password);
    result_user = query_result.AsSingleRow<models::User>(
        userver::storages::postgres::kRowTag);
  } catch (const userver::storages::postgres::UniqueViolation& ex) {
    auto& response = request.GetHttpResponse();
    response.SetStatus(userver::server::http::HttpStatus::kUnprocessableEntity);
    return utils::error::MakeError(ex.GetServerMessage().GetConstraint(),
                                   ex.GetServerMessage().GetDetail());
  }

  userver::formats::json::ValueBuilder builder;
  builder["user"] = result_user;
  return builder.ExtractValue();
}

}  // namespace real_medium::handlers::users::post
