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

// struct User

class AuthCheckerDigestBaseStandalone : public AuthCheckerDigestBase {
 public:
  AuthCheckerDigestBaseStandalone(const AuthDigestSettings& digest_settings,
                                  Realm&& realm);

  [[nodiscard]] bool SupportsUserAuth() const noexcept override { return true; }

  std::optional<UserData> GetUserData(const std::string& username) const override;
  void SetUserData(const std::string& username, const Nonce& nonce,
                    std::int32_t nonce_count,
                    TimePoint nonce_creation_time) const override;
  void PushUnnamedNonce(const Nonce& nonce,
                                std::chrono::milliseconds nonce_ttl) const override;
  std::optional<TimePoint> GetUnnamedNonceCreationTime(
      const Nonce& nonce) const override;
  virtual std::optional<UserData::HA1> GetHA1(const std::string& username) const = 0;


 private:
  mutable rcu::RcuMap<Username, UserData> user_data_;
  mutable rcu::RcuMap<Nonce, TimePoint> unnamed_nonces_;
};

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END