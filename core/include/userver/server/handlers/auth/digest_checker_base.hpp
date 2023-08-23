#pragma once

/// @file userver/server/handlers/auth/auth_digest_checker_base.hpp
/// @brief @copybrief server::handlers::auth::DigestCheckerBase

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

/// Used for data hashing and "nonce" generating.
class DigestHasher final {
 public:
  /// Constructor from the hash algorithm name from "crypto" namespace.
  /// Subsequently, all methods of the class will use this algorithm for
  /// hashing.
  DigestHasher(std::string_view algorithm);

  /// Returns "nonce" directive value in hexadecimal format.
  std::string GenerateNonce() const;

  /// Returns data hashed according to the specified in constructor
  /// algorithm.
  std::string GetHash(std::string_view data) const;

 private:
  using HashAlgorithm = std::function<std::string(
      std::string_view, crypto::hash::OutputEncoding)>;
  HashAlgorithm hash_algorithm_;
};

/// Contains information about the user.
struct UserData final {
  using HA1 = utils::NonLoggable<class HA1Tag, std::string>;

  UserData(HA1 ha1, std::string nonce, TimePoint timestamp,
           std::int64_t nonce_count = 0);

  HA1 ha1;
  std::string nonce;
  TimePoint timestamp;
  std::int64_t nonce_count{};
};

/// @ingroup userver_base_classes
///
/// @brief Base class for digest authentication checkers. Implements a
/// digest-authentication logic.
class DigestCheckerBase : public AuthCheckerBase {
 public:
  /// Assepts digest-authentication settings from
  /// @ref server::handlers::auth::DigestCheckerSettingsComponent and "realm"
  /// from handler config in static_config.yaml.
  DigestCheckerBase(const AuthDigestSettings& digest_settings,
                    std::string&& realm);

  DigestCheckerBase(const DigestCheckerBase&) = delete;
  DigestCheckerBase(DigestCheckerBase&&) = delete;
  DigestCheckerBase& operator=(const DigestCheckerBase&) = delete;
  DigestCheckerBase& operator=(DigestCheckerBase&&) = delete;

  ~DigestCheckerBase() override;

  /// The main checking function that is called for each request.
  [[nodiscard]] AuthCheckResult CheckAuth(
      const http::HttpRequest& request,
      request::RequestContext& request_context) const final;

  /// Returns "true" if the checker is allowed to write authentication
  /// information about the user to the RequestContext.
  [[nodiscard]] bool SupportsUserAuth() const noexcept override { return true; }

  /// The implementation should return std::nullopt if the user is not
  /// registered. If the user is registered, but he is not in storage, the
  /// implementation should create him with invalid data to avoids extra round
  /// trips for authentication challenges.
  virtual std::optional<UserData> FetchUserData(
      const std::string& username) const = 0;

  /// Sets user authentication data to storage.
  virtual void SetUserData(std::string username, std::string nonce,
                           std::int64_t nonce_count,
                           TimePoint nonce_creation_time) const = 0;

  /// Pushes "nonce" not tied to username to "Nonce Pool".
  virtual void PushUnnamedNonce(std::string nonce) const = 0;

  /// Returns "nonce" creation time from "Nonce Pool" if exists.
  virtual std::optional<TimePoint> GetUnnamedNonceCreationTime(
      const std::string& nonce) const = 0;

  /// @cond
  enum class ValidateResult { kOk, kWrongUserData, kDuplicateRequest };
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

  AuthCheckResult StartNewAuthSession(std::string username, std::string&& nonce,
                                      bool stale,
                                      http::HttpResponse& response) const;

  const std::string qops_;
  const std::string realm_;
  const std::string domains_;
  std::string_view algorithm_;
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
