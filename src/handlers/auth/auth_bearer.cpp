#include "auth_bearer.hpp"
#include "db/sql.hpp"
#include "utils/jwt.hpp"

#include <algorithm>

#include <userver/http/common_headers.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/component.hpp>

namespace real_medium::auth {

class AuthCheckerBearer final
    : public userver::server::handlers::auth::AuthCheckerBase {
 public:
  using AuthCheckResult = userver::server::handlers::auth::AuthCheckResult;

  AuthCheckerBearer(
      const userver::components::ComponentContext& component_context)
      : pg_cluster_(component_context
                        .FindComponent<userver::components::Postgres>(
                            "realmedium-database")
                        .GetCluster()) {}

  [[nodiscard]] AuthCheckResult CheckAuth(
      const userver::server::http::HttpRequest& request,
      userver::server::request::RequestContext& request_context) const override;

  [[nodiscard]] bool SupportsUserAuth() const noexcept override { return true; }

 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;
};

AuthCheckerBearer::AuthCheckResult AuthCheckerBearer::CheckAuth(
    const userver::server::http::HttpRequest& request,
    userver::server::request::RequestContext& request_context) const {
  const auto& auth_value =
      request.GetHeader(userver::http::headers::kAuthorization);
  if (auth_value.empty()) {
    return AuthCheckResult{
        AuthCheckResult::Status::kTokenNotFound,
        {},
        "Empty 'Authorization' header",
        userver::server::handlers::HandlerErrorCode::kUnauthorized};
  }

  const auto bearer_sep_pos = auth_value.find(' ');
  if (bearer_sep_pos == std::string::npos ||
      std::string_view{auth_value.data(), bearer_sep_pos} != "Bearer") {
    return AuthCheckResult{
        AuthCheckResult::Status::kTokenNotFound,
        {},
        "'Authorization' header should have 'Bearer some-token' format",
        userver::server::handlers::HandlerErrorCode::kUnauthorized};
  }
  std::string_view token{auth_value.data() + bearer_sep_pos + 1};
  auto payload = utils::jwt::DecodeJWT(token);
  auto id = payload.get_claim_value<std::string>("id");

  const auto res =
      pg_cluster_->Execute(userver::storages::postgres::ClusterHostType::kSlave,
                           sql::kFindUserById.data(), id);
  if (res.IsEmpty()) {
    return AuthCheckResult{
        AuthCheckResult::Status::kTokenNotFound,
        {},
        "Invalid user auth",
        userver::server::handlers::HandlerErrorCode::kUnauthorized};
  }

  request_context.SetData("id", id);
  return {};
}

userver::server::handlers::auth::AuthCheckerBasePtr CheckerFactory::operator()(
    const userver::components::ComponentContext& context,
    const userver::server::handlers::auth::HandlerAuthConfig& auth_config,
    const userver::server::handlers::auth::AuthCheckerSettings&) const {
  return std::make_shared<AuthCheckerBearer>(context);
}

}  // namespace real_medium::auth