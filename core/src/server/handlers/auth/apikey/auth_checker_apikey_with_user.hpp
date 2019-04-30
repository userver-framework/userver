#pragma once

#include <server/handlers/auth/apikey/auth_checker_apikey.hpp>

namespace server::handlers::auth::apikey {

class AuthCheckerApiKeyWithUser final : public AuthCheckerApiKey {
 public:
  AuthCheckerApiKeyWithUser(const HandlerAuthConfig& auth_config,
                            const AuthCheckerSettings& settings);

  [[nodiscard]] AuthCheckResult CheckAuth(
      const http::HttpRequest& request,
      request::RequestContext& context) const override;

  [[nodiscard]] bool SupportsUserAuth() const noexcept override { return true; }
};

}  // namespace server::handlers::auth::apikey
