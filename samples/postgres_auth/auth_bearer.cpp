/// [auth checker declaration]
#include "auth_bearer.hpp"
#include "user_info_cache.hpp"

#include <algorithm>

namespace samples::pg {

class AuthCheckerBearer final : public server::handlers::auth::AuthCheckerBase {
 public:
  using AuthCheckResult = server::handlers::auth::AuthCheckResult;

  AuthCheckerBearer(const AuthCache& auth_cache,
                    std::vector<server::auth::UserScope> required_scopes)
      : auth_cache_(auth_cache), required_scopes_(std::move(required_scopes)) {}

  [[nodiscard]] AuthCheckResult CheckAuth(
      const server::http::HttpRequest& request,
      server::request::RequestContext& request_context) const override;

  [[nodiscard]] bool SupportsUserAuth() const noexcept override { return true; }

 private:
  const AuthCache& auth_cache_;
  const std::vector<server::auth::UserScope> required_scopes_;
};
/// [auth checker declaration]

/// [auth checker definition 1]
AuthCheckerBearer::AuthCheckResult AuthCheckerBearer::CheckAuth(
    const server::http::HttpRequest& request,
    server::request::RequestContext& request_context) const {
  const auto& auth_value = request.GetHeader("Authorization");
  if (auth_value.empty()) {
    return AuthCheckResult{AuthCheckResult::Status::kTokenNotFound,
                           {},
                           "Empty 'Authorization' header",
                           server::handlers::HandlerErrorCode::kUnauthorized};
  }
  /// [auth checker definition 1]

  /// [auth checker definition 2]
  const auto bearer_sep_pos = auth_value.find(' ');
  if (bearer_sep_pos == std::string::npos ||
      std::string_view{auth_value.data(), bearer_sep_pos} != "Bearer") {
    return AuthCheckResult{
        AuthCheckResult::Status::kTokenNotFound,
        {},
        "'Authorization' header should have 'Bearer some-token' format",
        server::handlers::HandlerErrorCode::kUnauthorized};
  }
  /// [auth checker definition 2]

  /// [auth checker definition 3]
  const server::auth::UserAuthInfo::Ticket token{auth_value.data() +
                                                 bearer_sep_pos + 1};
  const auto cache_snapshot = auth_cache_.Get();

  auto it = cache_snapshot->find(token);
  if (it == cache_snapshot->end()) {
    return AuthCheckResult{AuthCheckResult::Status::kForbidden};
  }
  /// [auth checker definition 3]

  /// [auth checker definition 4]
  const UserDbInfo& info = it->second;
  for (const auto& scope : required_scopes_) {
    if (std::find(info.scopes.begin(), info.scopes.end(), scope.GetValue()) ==
        info.scopes.end()) {
      return AuthCheckResult{AuthCheckResult::Status::kForbidden,
                             {},
                             "No '" + scope.GetValue() + "' permission"};
    }
  }
  /// [auth checker definition 4]

  /// [auth checker definition 5]
  request_context.SetData("name", info.name);
  return {};
}
/// [auth checker definition 5]

/// [auth checker factory definition]
server::handlers::auth::AuthCheckerBasePtr CheckerFactory::operator()(
    const ::components::ComponentContext& context,
    const server::handlers::auth::HandlerAuthConfig& auth_config,
    const server::handlers::auth::AuthCheckerSettings&) const {
  auto scopes = auth_config["scopes"].As<server::auth::UserScopes>({});
  const auto& auth_cache = context.FindComponent<AuthCache>();
  return std::make_shared<AuthCheckerBearer>(auth_cache, std::move(scopes));
}
/// [auth checker factory definition]

}  // namespace samples::pg
