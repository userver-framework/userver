#pragma once

/// @file userver/server/handlers/auth/digest/auth_checker_base.hpp
/// @brief @copybrief server::handlers::auth::digest::AuthCheckerBase

#include <userver/server/handlers/auth/auth_checker_base.hpp>

#include <chrono>
#include <functional>
#include <optional>
#include <random>
#include <string_view>

#include <userver/crypto/hash.hpp>
#include <userver/rcu/rcu_map.hpp>
#include <userver/server/handlers/auth/digest/auth_checker_settings.hpp>
#include <userver/server/handlers/auth/digest/directives_parser.hpp>
#include <userver/server/http/http_request.hpp>
#include <userver/server/http/http_response.hpp>
#include <userver/server/http/http_status.hpp>
#include <userver/server/request/request_context.hpp>
#include <userver/storages/secdist/secdist.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth::digest {

using TimePoint = std::chrono::time_point<std::chrono::system_clock>;
using SecdistConfig = storages::secdist::SecdistConfig;
using ServerDigestAuthSecret =
    utils::NonLoggable<class DigestSecretKeyTag, std::string>;

/// Used for data hashing and "nonce" generating.
class Hasher final {
 public:
  /// Constructor from the hash algorithm name from "crypto" namespace
  /// to be used for hashing and storages::secdist::SecdistConfig containing a
  /// server secret key `http_server_digest_auth_secret`
  /// to be used for "nonce" generating.
  Hasher(std::string_view algorithm, const SecdistConfig& secdist_config);

  /// Returns "nonce" directive value in hexadecimal format.
  std::string GenerateNonce(std::string_view etag) const;

  /// Returns data hashed according to the specified in constructor
  /// algorithm.
  std::string GetHash(std::string_view data) const;

 private:
  using HashAlgorithm = std::function<std::string(
      std::string_view, crypto::hash::OutputEncoding)>;
  HashAlgorithm hash_algorithm_;
  const SecdistConfig& secdist_config_;
};

/// Contains information about the user.
struct UserData final {
  using HA1 = utils::NonLoggable<class HA1Tag, std::string>;

  UserData(HA1 ha1, std::string nonce, TimePoint timestamp,
           std::int64_t nonce_count);

  HA1 ha1;
  std::string nonce;
  TimePoint timestamp;
  std::int64_t nonce_count{};
};

/// @ingroup userver_base_classes
///
/// @brief Base class for digest authentication checkers. Implements a
/// digest-authentication logic.
class AuthCheckerBase : public auth::AuthCheckerBase {
 public:
  /// Assepts digest-authentication settings from
  /// @ref server::handlers::auth::DigestCheckerSettingsComponent and "realm"
  /// from handler config in static_config.yaml.
  AuthCheckerBase(const AuthCheckerSettings& digest_settings,
                  std::string&& realm, const SecdistConfig& secdist_config);

  AuthCheckerBase(const AuthCheckerBase&) = delete;
  AuthCheckerBase(AuthCheckerBase&&) = delete;
  AuthCheckerBase& operator=(const AuthCheckerBase&) = delete;
  AuthCheckerBase& operator=(AuthCheckerBase&&) = delete;

  ~AuthCheckerBase() override;

  /// The main checking function that is called for each request.
  [[nodiscard]] AuthCheckResult CheckAuth(
      const http::HttpRequest& request,
      request::RequestContext& request_context) const final;

  /// Returns "true" if the checker is allowed to write authentication
  /// information about the user to the RequestContext.
  [[nodiscard]] bool SupportsUserAuth() const noexcept override { return true; }

  /// The implementation should return std::nullopt if the user is not
  /// registered. If the user is registered, but he is not in storage, the
  /// implementation can create him with arbitrary data.
  virtual std::optional<UserData> FetchUserData(
      const std::string& username) const = 0;

  /// Sets user authentication data to storage.
  virtual void SetUserData(const std::string& username,
                           const std::string& nonce, std::int64_t nonce_count,
                           TimePoint nonce_creation_time) const = 0;

  /// Pushes "nonce" not tied to username to "Nonce Pool".
  virtual void PushUnnamedNonce(std::string nonce) const = 0;

  /// Returns "nonce" creation time from "Nonce Pool" if exists.
  virtual std::optional<TimePoint> GetUnnamedNonceCreationTime(
      const std::string& nonce) const = 0;

  /// @cond
  enum class ValidateResult { kOk, kWrongUserData, kDuplicateRequest };
  ValidateResult ValidateUserData(const ContextFromClient& client_context,
                                  const UserData& user_data) const;
  /// @endcond
 private:
  std::string CalculateDigest(const UserData::HA1& ha1_non_loggable,
                              http::HttpMethod request_method,
                              const ContextFromClient& client_context) const;

  std::string ConstructAuthInfoHeader(const ContextFromClient& client_context,
                                      std::string_view etag) const;

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

  const Hasher digest_hasher_;

  const std::string authenticate_header_;
  const std::string authorization_header_;
  const std::string authenticate_info_header_;
  const http::HttpStatus unauthorized_status_;
};

}  // namespace server::handlers::auth::digest

USERVER_NAMESPACE_END
