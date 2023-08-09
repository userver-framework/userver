
#include "user_put.hpp"
#include "../../dto/user.hpp"
#include "../../models/user.hpp"
#include "validators/user_validators.hpp"
#include "db/sql.hpp"
#include <userver/crypto/hash.hpp>

namespace real_medium::handlers::users::put {

Handler::Handler(const userver::components::ComponentConfig& config,
                 const userver::components::ComponentContext& component_context)
    : HttpHandlerBase(config, component_context),
      pg_cluster_(component_context
                      .FindComponent<userver::components::Postgres>(
                          "realmedium-database")
                      .GetCluster()) {}

std::string Handler::HandleRequestThrow(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext&) const {
  /*
  ..
  get user_id by token
  ..
  */

  auto user_id = "3dff4620-c620-4372-9d34-8d44b6bbc041";

  auto user_change_data =
      userver::formats::json::FromString(request.RequestBody())["user"]
          .As<dto::UserUpdateDTO>();

  if (user_change_data.email.has_value() &&
      !validators::ValidateEmail(user_change_data.email.value())) {
    auto& response = request.GetHttpResponse();
    response.SetStatus(userver::server::http::HttpStatus::kUnprocessableEntity);
    return {};
  }

  std::optional<std::string> password_hash = std::nullopt;
  if (user_change_data.password.has_value()){   

    if(!validators::ValidatePassword(user_change_data.password.value())) {
        auto& response = request.GetHttpResponse();
        response.SetStatus(userver::server::http::HttpStatus::kUnprocessableEntity);
        return {};
    }

    password_hash = userver::crypto::hash::Sha256(user_change_data.password.value());
  }

  const auto result = pg_cluster_->Execute(
      userver::storages::postgres::ClusterHostType::kMaster,
      sql::kUpdateUser.data(), 
      user_id, user_change_data.username,
      user_change_data.email, user_change_data.bio, user_change_data.image,
      password_hash);

  auto user_result_data = result.AsSingleRow<real_medium::models::User>(
      userver::storages::postgres::kRowTag);

  userver::formats::json::ValueBuilder builder;
  builder["user"] = user_result_data;

  return ToString(builder.ExtractValue());
}

}  // namespace real_medium::handlers::users::put