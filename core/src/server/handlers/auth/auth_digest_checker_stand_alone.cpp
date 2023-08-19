#include <optional>
#include <userver/server/handlers/auth/auth_digest_checker_stand_alone.hpp>

#include <string_view>

#include <userver/server/handlers/auth/auth_digest_settings.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {

AuthCheckerDigestBaseStandAlone::AuthCheckerDigestBaseStandAlone(const AuthDigestSettings& digest_settings, Realm realm)
  : AuthCheckerDigestBase(digest_settings, std::move(realm)) {};

std::optional<UserData> AuthCheckerDigestBaseStandAlone::GetUserData(const std::string& username) const {
  auto user_data_ptr = user_data_.Get(username);
  if (!user_data_ptr) return std::nullopt;
  LOG_DEBUG() << "USER IS OK";

  return *user_data_ptr;
}

void AuthCheckerDigestBaseStandAlone::SetUserData(const std::string& username,
                                                  UserData user_data) const {
    auto client_data_ptr = std::make_shared<UserData>(std::move(user_data));
    user_data_.InsertOrAssign(username, client_data_ptr);
}

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END