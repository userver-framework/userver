#include "auth_checker.hpp"

#include <userver/server/handlers/auth/auth_checker_factory.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {

namespace {
void ValidateAuthCheckersConsistency(
    const std::vector<AuthCheckerBasePtr>& auth_checkers) {
  const auto size = auth_checkers.size();
  if (size == 0) {
    return;
  }

  const bool sets_user = auth_checkers.front()->SupportsUserAuth();
  for (std::size_t i = 1; i < size; ++i) {
    if (sets_user != auth_checkers[i]->SupportsUserAuth()) {
      throw std::runtime_error(
          "Service authorization misconfigured. Mixing authorizations with "
          "and without user validation is not allowed.");
    }
  }
}
}  // namespace

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

  ValidateAuthCheckersConsistency(auth_checkers);

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
    if (check_result.status != AuthCheckResult::Status::kTokenNotFound) {
      RaiseForStatus(check_result);
      return;
    }
    if (first) {
      check_result_first = check_result;
      first = false;
    }
  }

  RaiseForStatus(check_result_first);
}

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END
