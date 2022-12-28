#pragma once

#include <userver/utest/using_namespace_userver.hpp>

/// [user info cache]
#include <vector>

#include <userver/cache/base_postgres_cache.hpp>
#include <userver/crypto/algorithm.hpp>
#include <userver/server/auth/user_auth_info.hpp>
#include <userver/storages/postgres/io/array_types.hpp>

namespace samples::pg {

struct UserDbInfo {
  server::auth::UserAuthInfo::Ticket token;
  std::int64_t user_id;
  std::vector<std::string> scopes;
  std::string name;
};

struct AuthCachePolicy {
  static constexpr std::string_view kName = "auth-pg-cache";

  using ValueType = UserDbInfo;
  static constexpr auto kKeyMember = &UserDbInfo::token;
  static constexpr const char* kQuery =
      "SELECT token, user_id, scopes, name FROM auth_schema.tokens";
  static constexpr const char* kUpdatedField = "updated";
  using UpdatedFieldType = storages::postgres::TimePointTz;

  // Using crypto::algorithm::StringsEqualConstTimeComparator to avoid timing
  // attack at find(token).
  using CacheContainer =
      std::unordered_map<server::auth::UserAuthInfo::Ticket, UserDbInfo,
                         std::hash<server::auth::UserAuthInfo::Ticket>,
                         crypto::algorithm::StringsEqualConstTimeComparator>;
};

using AuthCache = components::PostgreCache<AuthCachePolicy>;

}  // namespace samples::pg
/// [user info cache]
