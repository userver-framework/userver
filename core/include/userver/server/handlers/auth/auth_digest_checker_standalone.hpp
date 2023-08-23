#pragma once

#include "digest_checker_base.hpp"

#include <chrono>
#include <cstdint>
#include <functional>
#include <random>
#include <string_view>

#include <userver/cache/expirable_lru_cache.hpp>
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
  NonceInfo() = default;
  NonceInfo(const std::string& nonce, TimePoint expiration_time,
            std::int64_t nonce_count = 0)
      : nonce(nonce),
        expiration_time(expiration_time),
        nonce_count(nonce_count) {}
  std::string nonce;
  TimePoint expiration_time;
  std::int64_t nonce_count{};
};

class AuthCheckerDigestBaseStandalone : public DigestCheckerBase {
 public:
  AuthCheckerDigestBaseStandalone(const AuthDigestSettings& digest_settings,
                                  std::string&& realm);

  [[nodiscard]] bool SupportsUserAuth() const noexcept override { return true; }

  std::optional<UserData> FetchUserData(
      const std::string& username) const override;
  void SetUserData(std::string username,
                           std::string nonce, std::int64_t nonce_count,
                           TimePoint nonce_creation_time) const override;

  void PushUnnamedNonce(std::string nonce) const override;
  std::optional<TimePoint> GetUnnamedNonceCreationTime(
      const std::string& nonce) const override;

  virtual std::optional<UserData::HA1> GetHA1(
      std::string_view username) const = 0;

 private:
  mutable rcu::RcuMap<std::string, concurrent::Variable<NonceInfo>> user_data_;
  mutable cache::ExpirableLruCache<std::string, TimePoint> unnamed_nonces_{1,
                                                                           1};
};

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END
