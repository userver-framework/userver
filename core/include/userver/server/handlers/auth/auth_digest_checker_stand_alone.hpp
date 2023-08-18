#pragma once

#include "auth_digest_checker_base.hpp"

#include <chrono>
#include <functional>
#include <random>
#include <string_view>

#include <userver/crypto/hash.hpp>
#include <userver/rcu/rcu_map.hpp>
#include <userver/server/handlers/auth/auth_digest_settings.hpp>
#include <userver/server/handlers/auth/auth_params_parsing.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_response.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/server/request/request_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {

class AuthCheckerDigestBaseStandAlone : public AuthCheckerDigestBase {
 public:
  using AuthCheckResult = server::handlers::auth::AuthCheckResult;

  AuthCheckerDigestBaseStandAlone(const AuthDigestSettings& digest_settings, Realm realm);

  [[nodiscard]] bool SupportsUserAuth() const noexcept override { return true; }

  std::optional<UserData> GetUserData(
      const std::string& username) const override;
  void SetUserData(const std::string& username,
                           UserData user_data) const override;

 private:
  mutable rcu::RcuMap<Username, UserData> user_data_;
};

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END