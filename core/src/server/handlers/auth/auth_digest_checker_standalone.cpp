#include <userver/server/handlers/auth/auth_digest_checker_standalone.hpp>

#include <memory>
#include <optional>
#include <string_view>

#include <userver/server/handlers/auth/auth_digest_settings.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {

AuthCheckerDigestBaseStandalone::AuthCheckerDigestBaseStandalone(
    const AuthDigestSettings& digest_settings, Realm&& realm)
    : AuthCheckerDigestBase(digest_settings, std::move(realm)){};

UserData AuthCheckerDigestBaseStandalone::GetUserData(
    const std::string& username) const {
  auto user_data_ptr = user_data_.Get(username);
  if (!user_data_ptr) return std::nullopt;
  LOG_DEBUG() << "User is OK";

  // If nonce_info is not found by username, we create NonceInfo for username,
  // push to user_data_ and return UserData
  NonceInfo nonce_info_temp{"", utils::datetime::Now(), 0};
  SetUserData(username, nonce_info_temp.nonce, nonce_info_temp.nonce_count,
              nonce_info_temp.nonce_creation_time);
  UserData user_data{ha1.value(), nonce_info_temp.nonce,
                     nonce_info_temp.nonce_creation_time,
                     nonce_info_temp.nonce_count};
  return user_data;
}

void AuthCheckerDigestBaseStandalone::SetUserData(const std::string& username,
                                                  UserData user_data) const {
  auto client_data_ptr = std::make_shared<UserData>(std::move(user_data));
  user_data_.InsertOrAssign(username, client_data_ptr);
}

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END