#include <userver/server/handlers/auth/digest/standalone_checker.hpp>

#include <memory>
#include <optional>
#include <string_view>

#include <userver/server/handlers/auth/digest/auth_checker_settings.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth::digest {

NonceInfo::NonceInfo()
    : expiration_time(utils::datetime::Now()), nonce_count(0) {}

NonceInfo::NonceInfo(const std::string& nonce, TimePoint expiration_time,
                     std::int64_t nonce_count)
    : nonce(nonce),
      expiration_time(expiration_time),
      nonce_count(nonce_count) {}

AuthStandaloneCheckerBase::AuthStandaloneCheckerBase(
    const AuthCheckerSettings& digest_settings, std::string&& realm,
    std::size_t ways, std::size_t way_size)
    : AuthCheckerBase(digest_settings, std::move(realm)),
      unnamed_nonces_(ways, way_size) {
  unnamed_nonces_.SetMaxLifetime(digest_settings.nonce_ttl);
}

std::optional<UserData> AuthStandaloneCheckerBase::FetchUserData(
    const std::string& username) const {
  if (username.empty()) {
    return std::nullopt;
  }

  auto ha1 = GetHA1(username);
  if (!ha1) {
    // If ha1 is not found, we return an empty UserData
    return std::nullopt;
  }

  auto nonce_info = user_data_.Get(username);
  if (nonce_info) {
    // If nonce_info is found by username, we return UserData
    auto nonce_info_ptr = nonce_info->Lock();
    UserData user_data{ha1.value(), nonce_info_ptr->nonce,
                       nonce_info_ptr->expiration_time,
                       nonce_info_ptr->nonce_count};
    return user_data;
  }

  // If nonce_info is not found by username, we create NonceInfo for username,
  // push to user_data_ and return UserData
  NonceInfo nonce_info_temp{};
  SetUserData(username, nonce_info_temp.nonce, nonce_info_temp.nonce_count,
              nonce_info_temp.expiration_time);
  UserData user_data{ha1.value(), nonce_info_temp.nonce,
                     nonce_info_temp.expiration_time,
                     nonce_info_temp.nonce_count};
  return user_data;
}

void AuthStandaloneCheckerBase::SetUserData(
    const std::string& username, const std::string& nonce,
    std::int64_t nonce_count, TimePoint nonce_creation_time) const {
  auto nonce_info = user_data_.Get(username);
  if (nonce_info) {
    // If the nonce_info exists, we update it
    auto user_data_ptr = nonce_info->Lock();
    *user_data_ptr = NonceInfo{nonce, nonce_creation_time, nonce_count};
  } else {
    // Else we create nonce_info and put it to user_data_
    auto nonce_info_new = std::make_shared<concurrent::Variable<NonceInfo>>(
        nonce, nonce_creation_time, nonce_count);
    user_data_.InsertOrAssign(username, nonce_info_new);
  }
}

void AuthStandaloneCheckerBase::PushUnnamedNonce(std::string nonce) const {
  unnamed_nonces_.Put(nonce, utils::datetime::Now());
}

std::optional<TimePoint> AuthStandaloneCheckerBase::GetUnnamedNonceCreationTime(
    const std::string& nonce) const {
  auto unnamed_nonce = unnamed_nonces_.GetOptionalNoUpdate(nonce);
  if (unnamed_nonce) {
    unnamed_nonces_.InvalidateByKey(nonce);
  }
  return unnamed_nonce;
}

}  // namespace server::handlers::auth::digest

USERVER_NAMESPACE_END
