/// [auth checker declaration]
#include "auth_digest.hpp"
#include "user_info.hpp"

#include "sql/queries.hpp"

#include <algorithm>
#include <optional>
#include <string_view>

#include <userver/http/common_headers.hpp>
#include <userver/server/handlers/auth/digest/auth_checker_base.hpp>
#include <userver/server/handlers/auth/digest/auth_checker_settings_component.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/cluster_types.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/storages/postgres/result_set.hpp>
#include <userver/storages/secdist/component.hpp>
#include <userver/utils/datetime.hpp>

namespace samples::digest_auth {

using UserData = server::handlers::auth::digest::UserData;
using HA1 = server::handlers::auth::digest::UserData::HA1;
using SecdistConfig = storages::secdist::SecdistConfig;
using TimePoint = std::chrono::time_point<std::chrono::system_clock>;

class AuthChecker final
    : public server::handlers::auth::digest::AuthCheckerBase {
 public:
  using AuthCheckResult = server::handlers::auth::AuthCheckResult;
  using AuthDigestSettings =
      server::handlers::auth::digest::AuthCheckerSettings;

  AuthChecker(const AuthDigestSettings& digest_settings, std::string realm,
              const ::components::ComponentContext& context,
              const SecdistConfig& secdist_config)
      : server::handlers::auth::digest::AuthCheckerBase(
            digest_settings, std::move(realm), secdist_config),
        pg_cluster_(context.FindComponent<components::Postgres>("auth-database")
                        .GetCluster()),
        nonce_ttl_(digest_settings.nonce_ttl) {}

  std::optional<UserData> FetchUserData(
      const std::string& username) const override;

  void SetUserData(const std::string& username, const std::string& nonce,
                   std::int64_t nonce_count,
                   TimePoint nonce_creation_time) const override;

  void PushUnnamedNonce(std::string nonce) const override;

  std::optional<TimePoint> GetUnnamedNonceCreationTime(
      const std::string& nonce) const override;

 private:
  storages::postgres::ClusterPtr pg_cluster_;
  const std::chrono::milliseconds nonce_ttl_;
};
/// [auth checker declaration]

/// [auth checker definition 1]
std::optional<UserData> AuthChecker::FetchUserData(
    const std::string& username) const {
  storages::postgres::ResultSet res =
      pg_cluster_->Execute(storages::postgres::ClusterHostType::kSlave,
                           uservice_dynconf::sql::kSelectUser, username);

  if (res.IsEmpty()) return std::nullopt;

  auto user_db_info = res.AsSingleRow<UserDbInfo>(storages::postgres::kRowTag);
  return UserData{HA1{user_db_info.ha1}, user_db_info.nonce,
                  user_db_info.timestamp.GetUnderlying(),
                  user_db_info.nonce_count};
}
/// [auth checker definition 1]

/// [auth checker definition 2]
void AuthChecker::SetUserData(const std::string& username,
                              const std::string& nonce,
                              std::int64_t nonce_count,
                              TimePoint nonce_creation_time) const {
  pg_cluster_->Execute(storages::postgres::ClusterHostType::kMaster,
                       uservice_dynconf::sql::kUpdateUser, nonce,
                       storages::postgres::TimePointTz{nonce_creation_time},
                       nonce_count, username);
}
/// [auth checker definition 2]

/// [auth checker definition 3]
void AuthChecker::PushUnnamedNonce(std::string nonce) const {
  auto res = pg_cluster_->Execute(
      storages::postgres::ClusterHostType::kMaster,
      uservice_dynconf::sql::kInsertUnnamedNonce,
      storages::postgres::TimePointTz{utils::datetime::Now() - nonce_ttl_},
      nonce, storages::postgres::TimePointTz{utils::datetime::Now()});
}
/// [auth checker definition 3]

/// [auth checker definition 4]
std::optional<TimePoint> AuthChecker::GetUnnamedNonceCreationTime(
    const std::string& nonce) const {
  auto res =
      pg_cluster_->Execute(storages::postgres::ClusterHostType::kSlave,
                           uservice_dynconf::sql::kSelectUnnamedNonce, nonce);

  if (res.IsEmpty()) return std::nullopt;

  return res.AsSingleRow<storages::postgres::TimePointTz>().GetUnderlying();
}
/// [auth checker definition 4]

/// [auth checker factory definition]
server::handlers::auth::AuthCheckerBasePtr CheckerFactory::operator()(
    const ::components::ComponentContext& context,
    const server::handlers::auth::HandlerAuthConfig& auth_config,
    const server::handlers::auth::AuthCheckerSettings&) const {
  const auto& digest_auth_settings =
      context
          .FindComponent<
              server::handlers::auth::digest::AuthCheckerSettingsComponent>()
          .GetSettings();

  return std::make_shared<AuthChecker>(
      digest_auth_settings, auth_config["realm"].As<std::string>({}), context,
      context.FindComponent<components::Secdist>().Get());
}
/// [auth checker factory definition]

server::handlers::auth::AuthCheckerBasePtr CheckerProxyFactory::operator()(
    const ::components::ComponentContext& context,
    const server::handlers::auth::HandlerAuthConfig& auth_config,
    const server::handlers::auth::AuthCheckerSettings&) const {
  const auto& digest_auth_settings =
      context
          .FindComponent<
              server::handlers::auth::digest::AuthCheckerSettingsComponent>(
              "auth-digest-checker-settings-proxy")
          .GetSettings();

  return std::make_shared<AuthChecker>(
      digest_auth_settings, auth_config["realm"].As<std::string>({}), context,
      context.FindComponent<components::Secdist>().Get());
}

}  // namespace samples::digest_auth
