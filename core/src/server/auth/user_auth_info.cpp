#include <userver/server/auth/user_auth_info.hpp>

#include <algorithm>
#include <stdexcept>

#include <userver/utils/assert.hpp>

USERVER_NAMESPACE_BEGIN

namespace server::auth {

namespace {

const std::string kRequestContextKeyUserAuthInfo = "auth::user_info";

}  // namespace

UserAuthInfo::UserAuthInfo(UserId default_id, UserEnv env,
                           UserProvider provider)
    : default_id_{default_id},
      ids_{default_id},
      user_env_(env),
      user_provider_(provider) {}

UserAuthInfo::UserAuthInfo(UserId default_id, Ticket user_ticket, UserEnv env,
                           UserProvider provider)
    : default_id_{default_id},
      ids_{default_id},
      user_ticket_{std::move(user_ticket)},
      user_env_(env),
      user_provider_(provider) {}

UserAuthInfo::UserAuthInfo(UserId default_id, UserIds ids, UserScopes scopes,
                           UserEnv env, UserProvider provider)
    : default_id_(default_id),
      ids_(std::move(ids)),
      scopes_(std::move(scopes)),
      user_env_(env),
      user_provider_(provider) {}

UserAuthInfo::UserAuthInfo(UserId default_id, UserIds ids, UserScopes scopes,
                           Ticket user_ticket, UserEnv env,
                           UserProvider provider)
    : default_id_(default_id),
      ids_(std::move(ids)),
      scopes_(std::move(scopes)),
      user_ticket_{std::move(user_ticket)},
      user_env_(env),
      user_provider_(provider) {}

UserId UserAuthInfo::GetDefaultUserId() const { return default_id_; }

const UserIds& UserAuthInfo::GetUserIds() const { return ids_; }

const std::optional<UserScopes>& UserAuthInfo::GetUserScopesOptional() const {
  return scopes_;
}

const std::optional<UserAuthInfo::Ticket>& UserAuthInfo::GetTicketOptional()
    const {
  return user_ticket_;
}

void UserAuthInfo::Set(server::request::RequestContext& request_context,
                       UserAuthInfo&& info) {
  request_context.SetData(kRequestContextKeyUserAuthInfo, std::move(info));
}

const UserAuthInfo& GetUserAuthInfo(
    const server::request::RequestContext& request_context) {
  const auto* pauth_info = request_context.GetDataOptional<UserAuthInfo>(
      kRequestContextKeyUserAuthInfo);

  UASSERT_MSG(
      pauth_info,
      "No user info found in request context. Make sure that handler "
      "has type 'tvm2-with-user-ticket' or 'apikey-with-user' for 'auth'");
  if (!pauth_info) {
    throw std::runtime_error("No user info found in request context");
  }

  return *pauth_info;
}

}  // namespace server::auth

USERVER_NAMESPACE_END
