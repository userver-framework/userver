#include <userver/server/handlers/auth/auth_digest_checker_standalone.hpp>

#include <optional>
#include <string_view>

#include <userver/server/handlers/auth/auth_digest_settings.hpp>
#include "userver/utils/datetime.hpp"
#include <userver/concurrent/mpsc_queue.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::handlers::auth {

AuthCheckerDigestBaseStandalone::AuthCheckerDigestBaseStandalone(
    const AuthDigestSettings& digest_settings, Realm&& realm)
    : AuthCheckerDigestBase(digest_settings, std::move(realm)){};

std::optional<UserData> AuthCheckerDigestBaseStandalone::GetUserData(
    const std::string& username) const {
  auto user_data_ptr = user_data_.Get(username);
  if (!user_data_ptr) {
    UserData user_data{"", utils::datetime::Now()};
    SetUserData(username, std::move(user_data));
  }

  return *user_data_ptr;
}

void AuthCheckerDigestBaseStandalone::SetUserData(const std::string& username, const Nonce& nonce,
                    std::int32_t nonce_count,
                    TimePoint nonce_creation_time) const {
    
    //  auto ha1 = GetHA1(username);
//   auto client_data_ptr = std::make_shared<UserData>(std::move(user_data));
//   user_data_.InsertOrAssign(username, client_data_ptr);
}

void PushUnnamedNonce(const Nonce& nonce,
                                std::chrono::milliseconds nonce_ttl) const {
    
}
std::optional<TimePoint> GetUnnamedNonceCreationTime(
    const Nonce& nonce) const override;

}  // namespace server::handlers::auth

USERVER_NAMESPACE_END