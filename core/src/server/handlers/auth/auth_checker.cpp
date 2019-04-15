#include "auth_checker.hpp"

#include <server/handlers/auth/auth_checker_factory.hpp>

namespace server {
namespace handlers {
namespace auth {

std::vector<AuthCheckerBasePtr> CreateAuthCheckers(
    const components::ComponentContext& component_context,
    const HandlerConfig& config, const AuthCheckerSettings& settings) {
  if (!config.auth) return {};

  std::vector<AuthCheckerBasePtr> auth_checkers;

  for (const auto& auth_type : config.auth->GetTypes()) {
    const auto& factory = GetAuthCheckerFactory(auth_type);
    auth_checkers.emplace_back(
        factory(component_context, *config.auth, settings));
  }
  return auth_checkers;
}

void CheckAuth(const std::vector<AuthCheckerBasePtr>& auth_checkers,
               const http::HttpRequest& http_request,
               request::RequestContext& context) {
  if (auth_checkers.empty()) return;

  auth::AuthCheckResult check_result_first;

  bool first = true;
  for (const auto& auth_checker : auth_checkers) {
    auto check_result = auth_checker->CheckAuth(http_request, context);
    if (check_result.GetStatus() != AuthCheckResult::Status::kTokenNotFound) {
      check_result.RaiseForStatus();
      return;
    }
    if (first) {
      check_result_first = check_result;
      first = false;
    }
  }

  check_result_first.RaiseForStatus();
}

}  // namespace auth
}  // namespace handlers
}  // namespace server
