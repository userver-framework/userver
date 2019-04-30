#include "auth_checker_apikey_with_user.hpp"

#include <boost/lexical_cast.hpp>

#include <http/common_headers.hpp>
#include <server/auth/user_auth_info.hpp>

namespace server::handlers::auth::apikey {

AuthCheckerApiKeyWithUser::AuthCheckerApiKeyWithUser(
    const HandlerAuthConfig& auth_config, const AuthCheckerSettings& settings)
    : AuthCheckerApiKey(auth_config, settings) {}

AuthCheckResult AuthCheckerApiKeyWithUser::CheckAuth(
    const http::HttpRequest& request, request::RequestContext& context) const {
  using ::http::headers::kXYandexUid;
  const auto& uid = request.GetHeader(kXYandexUid);

  // kXYandexUid is required for all the user authorizations. If it is missing,
  // then any user authourization should fail.
  if (uid.empty()) {
    // Return early if there's no uid.
    std::string reason =
        "missing or empty " + std::string{kXYandexUid} + " header";
    return AuthCheckResult{AuthCheckResult::Status::kForbidden, reason, reason};
  }

  server::auth::UserAuthInfo info{
      server::auth::UserId{boost::lexical_cast<std::uint64_t>(uid)}};
  SetUserAuthInfo(context, std::move(info));

  return AuthCheckerApiKey::CheckAuth(request, context);
}

}  // namespace server::handlers::auth::apikey
