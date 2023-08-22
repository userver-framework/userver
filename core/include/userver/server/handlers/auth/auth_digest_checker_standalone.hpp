#pragma once

#include "auth_digest_checker_base.hpp"

#include <chrono>
#include <cstdint>
#include <functional>
#include <random>
#include <string_view>

#include <userver/concurrent/mpsc_queue.hpp>
#include <userver/concurrent/variable.hpp>
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

struct NonceInfo final {
  Nonce nonce;
  TimePoint nonce_creation_time;
  std::int32_t nonce_count;
};

class AuthCheckerDigestBaseStandalone : public AuthCheckerDigestBase {
 public:
  AuthCheckerDigestBaseStandalone(const AuthDigestSettings& digest_settings,
                                  Realm&& realm);

  [[nodiscard]] bool SupportsUserAuth() const noexcept override { return true; }

  std::optional<UserData> GetUserData(
      const std::string& username) const override;
  void SetUserData(const std::string& username, const Nonce& nonce,
                   std::int32_t nonce_count,
                   TimePoint nonce_creation_time) const override;
  virtual std::optional<UserData::HA1> GetHA1(
      const std::string& username) const = 0;

 private:
  mutable rcu::RcuMap<Username, concurrent::Variable<NonceInfo>> user_data_;
};

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END