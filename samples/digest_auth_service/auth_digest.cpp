#include "auth_digest.hpp"
#include "user_info.hpp"
#include "userver/logging/log.hpp"
#include "userver/server/handlers/auth/auth_checker_settings.hpp"
#include "userver/server/handlers/auth/digest_checker_base.hpp"
#include "userver/storages/postgres/cluster_types.hpp"
#include "userver/storages/postgres/component.hpp"
#include "userver/storages/postgres/io/row_types.hpp"
#include "userver/storages/postgres/postgres_fwd.hpp"
#include "userver/storages/postgres/query.hpp"
#include "userver/storages/postgres/result_set.hpp"
#include "userver/utils/datetime.hpp"

#include <algorithm>
#include <optional>
#include <string_view>
#include <userver/http/common_headers.hpp>
#include <userver/server/handlers/auth/digest_checker_settings_component.hpp>

namespace samples::pg {

using UserData = server::handlers::auth::UserData;
using HA1 = server::handlers::auth::UserData::HA1;

class AuthCheckerDigest final
    : public server::handlers::auth::DigestCheckerBase {
 public:
  using AuthCheckResult = server::handlers::auth::AuthCheckResult;
  using AuthDigestSettings =
      userver::server::handlers::auth::AuthDigestSettings;

  AuthCheckerDigest(const AuthDigestSettings& digest_settings,
                    std::string realm,
                    const ::components::ComponentContext& context)
      : server::handlers::auth::DigestCheckerBase(digest_settings,
                                                  std::move(realm)),
        pg_cluster_(
            context
                .FindComponent<userver::components::Postgres>("auth-database")
                .GetCluster()) {}

  std::optional<HA1> GetHA1(std::string_view username) const;

  std::optional<UserData> FetchUserData(
      std::string_view username) const override;

  void SetUserData(std::string username, std::string nonce,
                   std::int64_t nonce_count,
                   TimePoint nonce_creation_time) const override;

  void PushUnnamedNonce(std::string nonce,
                        std::chrono::milliseconds nonce_ttl) const override;

  std::optional<TimePoint> GetUnnamedNonceCreationTime(
      std::string_view nonce) const override;

 private:
  userver::storages::postgres::ClusterPtr pg_cluster_;

  const storages::postgres::Query kSelectUser{
      "SELECT username, nonce, timestamp, nonce_count, ha1 "
      "FROM auth_schema.users WHERE username=$1",
      storages::postgres::Query::Name{"select_user"}};

  const storages::postgres::Query kSelectHA1{
      "SELECT ha1 FROM auth_schema.users WHERE username=$1",
      storages::postgres::Query::Name{"select_ha1"}};

  const storages::postgres::Query kUpdateUser{
      "UPDATE auth_schema.users "
      "SET nonce=$1, timestamp=$2, nonce_count=$3 "
      "WHERE username=$4",
      storages::postgres::Query::Name{"update_user"}};

  const storages::postgres::Query kInsertUnnamedNonce{
      "WITH expired AS( "
      "  SELECT id FROM auth_schema.unnamed_nonce WHERE creation_time <= $1 "
      "LIMIT 1 "
      "), "
      "free_id AS ( "
      "SELECT COALESCE((SELECT id FROM expired LIMIT 1), "
      "nextval('nonce_id_seq')) AS id "
      ") "
      "INSERT INTO auth_schema.unnamed_nonce (id, nonce, creation_time) "
      "SELECT "
      "  free_id.id, "
      "  $2, "
      "  $3 "
      "FROM free_id "
      "ON CONFLICT (id) DO UPDATE SET "
      "  nonce=$2, "
      "  creation_time=$3 "
      "  WHERE auth_schema.unnamed_nonce.id=(SELECT free_id.id FROM free_id "
      "LIMIT 1) ",
      storages::postgres::Query::Name{"insert_unnamed_nonce"}};

  const storages::postgres::Query kSelectUnnamedNonce{
      "UPDATE auth_schema.unnamed_nonce SET nonce='' WHERE nonce=$1 "
      "RETURNING creation_time ",
      storages::postgres::Query::Name{"select_unnamed_nonce"}};
};

std::optional<HA1> AuthCheckerDigest::GetHA1(std::string_view username) const {
  storages::postgres::ResultSet res = pg_cluster_->Execute(
      storages::postgres::ClusterHostType::kSlave, kSelectHA1, username);

  return HA1{res.AsSingleRow<std::string>()};
}

std::optional<UserData> AuthCheckerDigest::FetchUserData(
    std::string_view username) const {
  auto ha1_opt = GetHA1(username);
  if (!ha1_opt.has_value()) return std::nullopt;

  storages::postgres::ResultSet res = pg_cluster_->Execute(
      storages::postgres::ClusterHostType::kSlave, kSelectUser, username);

  auto userDbInfo =
      res.AsSingleRow<UserDbInfo>(userver::storages::postgres::kRowTag);
  return UserData{ha1_opt.value(), userDbInfo.nonce, userDbInfo.timestamp,
                  userDbInfo.nonce_count};
}

void AuthCheckerDigest::SetUserData(std::string username, std::string nonce,
                                    std::int64_t nonce_count,
                                    TimePoint nonce_creation_time) const {
  pg_cluster_->Execute(storages::postgres::ClusterHostType::kMaster,
                       kUpdateUser, nonce, nonce_creation_time, nonce_count,
                       username);
}

void AuthCheckerDigest::PushUnnamedNonce(
    std::string nonce, std::chrono::milliseconds nonce_ttl) const {
  auto res = pg_cluster_->Execute(
      storages::postgres::ClusterHostType::kMaster, kInsertUnnamedNonce,
      utils::datetime::Now() - nonce_ttl, nonce, utils::datetime::Now());
}

std::optional<TimePoint> AuthCheckerDigest::GetUnnamedNonceCreationTime(
    std::string_view nonce) const {
  auto res = pg_cluster_->Execute(storages::postgres::ClusterHostType::kSlave,
                                  kSelectUnnamedNonce, nonce);

  if (res.IsEmpty()) return std::nullopt;

  return res.AsSingleRow<TimePoint>();
}

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

}  // namespace samples::pg
