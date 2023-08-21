#pragma once

/// @file userver/server/handlers/auth/auth_digest_checker_base.hpp
/// @brief @copybrief server::handlers::auth::AuthCheckerDigestBase

#include "auth_checker_base.hpp"

#include <chrono>
#include <functional>
#include <optional>
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

using TimePoint = std::chrono::time_point<std::chrono::system_clock>;

class DigestHasher final {
 public:
  DigestHasher(const std::string& algorithm);
  std::string Nonce() const;
  std::string GetHash(std::string_view data) const;

 private:
  using HashAlgorithm = std::function<std::string(
      std::string_view, crypto::hash::OutputEncoding)>;
  HashAlgorithm hash_algorithm_;
};

struct UserData final {
  using HA1 = utils::NonLoggable<class HA1Tag, std::string>;

  UserData() = default;
  UserData(HA1 ha1, const std::string& nonce, TimePoint timestamp,
           std::int32_t nonce_count = 0)
      : ha1(ha1),
        nonce(nonce),
        timestamp(timestamp),
        nonce_count(nonce_count) {}

  HA1 ha1;
  std::string nonce;
  TimePoint timestamp;
  std::int32_t nonce_count{};
};

enum class ValidateResult { kOk, kWrongUserData, kDuplicateRequest };

// clang-format off

/// @brief Authentication checker, that realise a digest-authentication.

// clang-format on

class AuthCheckerDigestBase : public AuthCheckerBase {
 public:
  AuthCheckerDigestBase(const AuthDigestSettings& digest_settings,
                        std::string&& realm);

  AuthCheckerDigestBase(const AuthCheckerDigestBase&) = delete;
  AuthCheckerDigestBase(AuthCheckerDigestBase&&) = delete;
  AuthCheckerDigestBase& operator=(const AuthCheckerDigestBase&) = delete;
  AuthCheckerDigestBase& operator=(AuthCheckerDigestBase&&) = delete;

  ~AuthCheckerDigestBase() override = default;

  [[nodiscard]] AuthCheckResult CheckAuth(
      const http::HttpRequest& request,
      request::RequestContext& request_context) const final;

  [[nodiscard]] bool SupportsUserAuth() const noexcept override { return true; }

  virtual std::optional<UserData> GetUserData(
      const std::string& username) const = 0;
  virtual void SetUserData(const std::string& username,
                           const std::string& nonce, std::int32_t nonce_count,
                           TimePoint nonce_creation_time) const = 0;

  virtual void PushUnnamedNonce(const std::string& nonce,
                                std::chrono::milliseconds nonce_ttl) const = 0;
  virtual std::optional<TimePoint> GetUnnamedNonceCreationTime(
      const std::string& nonce) const = 0;
  
  /// @cond
  ValidateResult ValidateUserData(const DigestContextFromClient& client_context,
                                  const UserData& user_data) const;
  /// @endcond
 private:
  std::string CalculateDigest(
      const UserData::HA1& ha1_non_loggable, http::HttpMethod request_method,
      const DigestContextFromClient& client_context) const;

  std::string ConstructAuthInfoHeader(
      const DigestContextFromClient& client_context) const;
  std::string ConstructResponseDirectives(std::string_view nonce,
                                          bool stale) const;
  AuthCheckResult StartNewAuthSession(const std::string& username,
                                      const std::string& nonce_from_client,
                                      bool stale,
                                      http::HttpResponse& response) const;

  const std::string qops_;
  const std::string realm_;
  const std::string domains_;
  const std::string& algorithm_;
  const bool is_session_;
  const bool is_proxy_;
  const std::chrono::milliseconds nonce_ttl_;

  const DigestHasher digest_hasher_;

  const std::string authenticate_header_;
  const std::string authorization_header_;
  const std::string authenticate_info_header_;
  const http::HttpStatus unauthorized_status_;
};

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END