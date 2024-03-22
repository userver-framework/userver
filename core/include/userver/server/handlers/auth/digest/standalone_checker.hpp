#pragma once

/// @file userver/server/handlers/auth/digest/standalone_checker.hpp
/// @brief @copybrief server::handlers::auth::digest::AuthStandaloneCheckerBase

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
#include <userver/server/handlers/auth/digest/auth_checker_base.hpp>
#include <userver/server/handlers/auth/digest/auth_checker_settings.hpp>
#include <userver/server/handlers/auth/digest/directives_parser.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_response.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/server/request/request_context.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth::digest {

struct NonceInfo final {
  NonceInfo();
  NonceInfo(const std::string& nonce, TimePoint expiration_time,
            std::int64_t nonce_count);
  std::string nonce;
  TimePoint expiration_time;
  std::int64_t nonce_count;
};

/// @ingroup userver_base_classes
///
/// @brief Class for digest authentication checker. Implements a stand-alone
/// digest-authentication logic.
class AuthStandaloneCheckerBase : public AuthCheckerBase {
 public:
  AuthStandaloneCheckerBase(const AuthCheckerSettings& digest_settings,
                            std::string&& realm,
                            const SecdistConfig& secdist_config,
                            std::size_t ways, std::size_t way_size);

  [[nodiscard]] bool SupportsUserAuth() const noexcept override { return true; }

  std::optional<UserData> FetchUserData(
      const std::string& username) const override;
  void SetUserData(const std::string& username, const std::string& nonce,
                   std::int64_t nonce_count,
                   TimePoint nonce_creation_time) const override;

  void PushUnnamedNonce(std::string nonce) const override;
  std::optional<TimePoint> GetUnnamedNonceCreationTime(
      const std::string& nonce) const override;

  virtual std::optional<UserData::HA1> GetHA1(
      std::string_view username) const = 0;

 private:
  using NonceCache = cache::ExpirableLruCache<std::string, TimePoint>;
  // potentially we store ALL user's data
  // great chance to occupy large block of memory
  mutable rcu::RcuMap<std::string, concurrent::Variable<NonceInfo>> user_data_;
  // cache for "unnamed" nonces,
  // i.e initial nonces not tied to any user
  mutable NonceCache unnamed_nonces_;
};

}  // namespace server::handlers::auth::digest

USERVER_NAMESPACE_END
