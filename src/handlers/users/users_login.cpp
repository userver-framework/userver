#include "users_login.hpp"

#include <userver/crypto/hash.hpp>
#include <userver/server/handlers/http_handler_base.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/components/component_config.hpp>
#include <userver/components/component_context.hpp>

#include "dto/user.hpp"
#include "validators/user_validators.hpp"
#include "models/user.hpp"
#include "db/sql.hpp"

namespace real_medium::handlers::users_login::post {

namespace {

class LoginUser final : public userver::server::handlers::HttpHandlerBase {
public:
  static constexpr std::string_view kName = "handler-login-user";

  LoginUser(const userver::components::ComponentConfig &config,
            const userver::components::ComponentContext &component_context)
      : HttpHandlerBase(config, component_context),
        pg_cluster_(
            component_context
                .FindComponent<userver::components::Postgres>("realmedium-database")
                .GetCluster()) {}

  std::string HandleRequestThrow(
      const userver::server::http::HttpRequest &request,
      userver::server::request::RequestContext &) const override {
    auto user_login = userver::formats::json::FromString(request.RequestBody())["user"].As<dto::UserLoginDTO>();

    if (!validators::ValidateEmail(user_login.email)) {
      auto &response = request.GetHttpResponse();
      response.SetStatus(userver::server::http::HttpStatus::kUnprocessableEntity);
      return {};
    }

    if (!validators::ValidatePassword(user_login.password)) {
      auto& response = request.GetHttpResponse();
      response.SetStatus(userver::server::http::HttpStatus::kUnprocessableEntity);
      return {};
    }

    auto password_hash = userver::crypto::hash::Sha256(user_login.password);

    auto userResult = pg_cluster_->Execute(
        userver::storages::postgres::ClusterHostType::kMaster,
        sql::kSelectUserByEmailAndPassword.data(),
        user_login.email, password_hash);

    if (userResult.IsEmpty()) {
      auto &response = request.GetHttpResponse();
      response.SetStatus(userver::server::http::HttpStatus::kNotFound);
      return {};
    }

    auto user =
        userResult.AsSingleRow<models::User>(userver::storages::postgres::kRowTag);

    userver::formats::json::ValueBuilder response;
    response["user"] = user;

    return userver::formats::json::ToString(response.ExtractValue());
  }

private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

} // namespace

void AppendLoginUser(userver::components::ComponentList &component_list) {
  component_list.Append<LoginUser>();
}

} // namespace bookmarker