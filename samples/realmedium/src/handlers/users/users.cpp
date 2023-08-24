#include <string>

#include "users.hpp"

#include <userver/crypto/hash.hpp>

#include "db/sql.hpp"
#include "dto/user.hpp"
#include "models/user.hpp"
#include "utils/errors.hpp"
#include "utils/make_error.hpp"
#include "validators/validators.hpp"

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
  dto::UserRegistrationDTO user_register =
      request_json["user"].As<dto::UserRegistrationDTO>();
  ;

  try {
    validator::validate(user_register);
  } catch (const utils::error::ValidationException& err) {
    request.SetResponseStatus(
        userver::server::http::HttpStatus::kUnprocessableEntity);
    return err.GetDetails();
  }

  auto hash_password =
      userver::crypto::hash::Sha256(user_register.password.value());
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
