#include <userver/server/handlers/auth/auth_digest_checker_standalone.hpp>

#include <memory>
#include <optional>
#include <string_view>

#include <userver/server/handlers/auth/auth_digest_settings.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {

AuthCheckerDigestBaseStandalone::AuthCheckerDigestBaseStandalone(
    const AuthDigestSettings& digest_settings, std::string&& realm)
    : DigestCheckerBase(digest_settings, std::move(realm)) {
  unnamed_nonces_.SetMaxLifetime(digest_settings.nonce_ttl);
};

std::optional<UserData> AuthCheckerDigestBaseStandalone::FetchUserData(
    const std::string& username) const {
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
  NonceInfo nonce_info_temp{"", utils::datetime::Now(), 0};
  SetUserData(username, nonce_info_temp.nonce, nonce_info_temp.nonce_count,
              nonce_info_temp.expiration_time);
  UserData user_data{ha1.value(), nonce_info_temp.nonce,
                     nonce_info_temp.expiration_time,
                     nonce_info_temp.nonce_count};
  return user_data;
}

void AuthCheckerDigestBaseStandalone::SetUserData(
    std::string username, std::string nonce, std::int64_t nonce_count,
    TimePoint nonce_creation_time) const {
  auto nonce_info = user_data_.Get(username);
  if (nonce_info) {
    // If the nonce_info exists, we update it
    auto user_data_ptr = user_data_[username]->Lock();
    *user_data_ptr = NonceInfo{nonce, nonce_creation_time, nonce_count};
  } else {
    // Else we create nonce_info and put it to user_data_
    auto nonce_info_new = std::make_shared<concurrent::Variable<NonceInfo>>(
        nonce, nonce_creation_time, nonce_count);
    user_data_.InsertOrAssign(username, nonce_info_new);
  }
}

void AuthCheckerDigestBaseStandalone::PushUnnamedNonce(
    std::string nonce) const {
  unnamed_nonces_.Put(nonce, static_cast<TimePoint>(
                                 userver::utils::datetime::Now()));
}

std::optional<TimePoint>
AuthCheckerDigestBaseStandalone::GetUnnamedNonceCreationTime(
  const std::string& nonce) const {
  auto unnamed_nonce = unnamed_nonces_.GetOptionalNoUpdate(nonce);
  if (unnamed_nonce) {
    unnamed_nonces_.InvalidateByKey(nonce);
  }
  return unnamed_nonce;
}

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END