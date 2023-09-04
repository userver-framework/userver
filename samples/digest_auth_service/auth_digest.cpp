/// [auth checker declaration]
#include "auth_digest.hpp"
#include "user_info.hpp"

#include "sql/queries.hpp"

#include <algorithm>
#include <optional>
#include <string_view>

#include <userver/http/common_headers.hpp>
#include <userver/server/handlers/auth/digest_checker_base.hpp>
#include <userver/server/handlers/auth/digest_checker_settings_component.hpp>
#include <userver/storages/postgres/cluster.hpp>
#include <userver/storages/postgres/cluster_types.hpp>
#include <userver/storages/postgres/component.hpp>
#include <userver/storages/postgres/result_set.hpp>
#include <userver/utils/datetime.hpp>

namespace samples::digest_auth {

using UserData = server::handlers::auth::UserData;
using HA1 = server::handlers::auth::UserData::HA1;
using TimePoint = std::chrono::time_point<std::chrono::system_clock>;

class AuthCheckerDigest final
    : public server::handlers::auth::DigestCheckerBase {
 public:
  using AuthCheckResult = server::handlers::auth::AuthCheckResult;
  using AuthDigestSettings = server::handlers::auth::AuthDigestSettings;

  AuthCheckerDigest(const AuthDigestSettings& digest_settings,
                    std::string realm,
                    const ::components::ComponentContext& context)
      : server::handlers::auth::DigestCheckerBase(digest_settings,
                                                  std::move(realm)),
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
std::optional<UserData> AuthCheckerDigest::FetchUserData(
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
void AuthCheckerDigest::SetUserData(const std::string& username,
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
void AuthCheckerDigest::PushUnnamedNonce(std::string nonce) const {
  auto res = pg_cluster_->Execute(
      storages::postgres::ClusterHostType::kMaster,
      uservice_dynconf::sql::kInsertUnnamedNonce,
      storages::postgres::TimePointTz{utils::datetime::Now() - nonce_ttl_},
      nonce, storages::postgres::TimePointTz{utils::datetime::Now()});
}
/// [auth checker definition 3]

/// [auth checker definition 4]
std::optional<TimePoint> AuthCheckerDigest::GetUnnamedNonceCreationTime(
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
              server::handlers::auth::DigestCheckerSettingsComponent>()
          .GetSettings();

  return std::make_shared<AuthCheckerDigest>(
      digest_auth_settings, auth_config["realm"].As<std::string>({}), context);
}
/// [auth checker factory definition]

}  // namespace samples::digest_auth
